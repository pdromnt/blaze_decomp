#include "sdl_music.h"
#include "blaze.h"
#include <SDL3/SDL.h>
#include <fluidsynth.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { LOAD,PLAY,STOP, RATE=44100, CHUNK=512, QUEUE_FRAMES=4096 };
typedef struct Request {
    struct Request *next;uint64_t sequence;unsigned operation,slot;int loop,cd;
    char path[1200];
} Request;
struct BlazeSdlMusic {
    SDL_Mutex *lock;SDL_Condition *wake;SDL_Thread *thread;
    Request *head,*tail;BlazeMusicStatus status;
    uint64_t intent,finished;int intent_slot,closing,muted,ready;
    char root[1024],bank[1200];
    /* Worker exclusively owns everything below. No synth/device calls under
       the publication lock, and no disk I/O or synthesis on the game thread. */
    fluid_settings_t *settings;fluid_synth_t *synth;fluid_player_t *slots[2];
    SDL_AudioStream *stream;int active,playing,draining,loop,last_tick;
    uint64_t play_sequence,starts,loops;
};
static void publish(BlazeSdlMusic *m,uint64_t completed,uint64_t finished,int failed) {
    SDL_LockMutex(m->lock);
    if(completed)m->status.completed=completed;
    if(finished)m->finished=finished;
    m->status.failed|=failed;m->status.active=m->active;m->status.playing=m->playing;
    m->status.valid[0]=m->slots[0]!=NULL;m->status.valid[1]=m->slots[1]!=NULL;
    m->status.starts=m->starts;m->status.loops=m->loops;
    SDL_UnlockMutex(m->lock);
}
static void stop_song(BlazeSdlMusic *m) {
    if(m->active>=0 && m->slots[m->active]) {
        int16_t discard[64*2];
        fluid_player_stop(m->slots[m->active]);
        /* Sample-timed stop sends its final all-sound-off on the next render.
           Consume it before a different slot can issue fresh note-ons. */
        fluid_synth_write_s16(m->synth,64,discard,0,2,discard,1,2);
    }
    fluid_synth_system_reset(m->synth);
    if(m->stream)SDL_ClearAudioStream(m->stream);
    m->playing=m->draining=0;
}
static void request(BlazeSdlMusic *m,Request *r) {
    if(r->operation==LOAD) {
        uint8_t *bytes=NULL;size_t size=0;fluid_player_t *player=NULL;
        if((int)r->slot==m->active)stop_song(m);
        if(m->slots[r->slot])delete_fluid_player(m->slots[r->slot]);
        m->slots[r->slot]=NULL;
        if(r->cd)fprintf(stderr,"SDL MIDI: CD playback is unsupported\n");
        else if(blaze_file_read(r->path,&bytes,&size) && size>=14 && !memcmp(bytes,"MThd",4)) {
            player=new_fluid_player(m->synth);
            if(player && fluid_player_add_mem(player,bytes,size)!=FLUID_OK) {delete_fluid_player(player);player=NULL;}
            m->slots[r->slot]=player;
        }
        if(!m->slots[r->slot])fprintf(stderr,"SDL MIDI: cannot load %s\n",r->path);
        free(bytes);
    } else if(r->operation==STOP)stop_song(m);
    else {
        fluid_player_t *p;
        stop_song(m);m->active=(int)r->slot;m->loop=r->loop;m->play_sequence=r->sequence;m->last_tick=0;
        p=m->slots[r->slot];
        if(p) {
            fluid_player_set_loop(p,r->loop?-1:1);fluid_player_seek(p,0);
            m->playing=fluid_player_play(p)==FLUID_OK;
            if(m->playing)++m->starts;
        }
    }
    publish(m,r->sequence,0,r->operation==LOAD && r->cd);
}
static int initialize(BlazeSdlMusic *m) {
    SDL_AudioSpec spec={SDL_AUDIO_S16,2,RATE};
    m->settings=new_fluid_settings();if(!m->settings)return 0;
    fluid_settings_setnum(m->settings,"synth.sample-rate",RATE);
    /* GeneralUser GS needs a higher music level against the original SFX. */
    fluid_settings_setnum(m->settings,"synth.gain",0.5);
    fluid_settings_setint(m->settings,"synth.cpu-cores",1);
    fluid_settings_setint(m->settings,"synth.threadsafe-api",0);
    fluid_settings_setstr(m->settings,"player.timing-source","sample");
    /* The original sequencer replays events on loops; do not inject an extra
       controller reset at every EOF. Scene changes reset explicitly above. */
    fluid_settings_setint(m->settings,"player.reset-synth",0);
    m->synth=new_fluid_synth(m->settings);
    if(!m->synth || fluid_synth_sfload(m->synth,m->bank,1)<0)return 0;
    if(!m->muted) {
        m->stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);
        if(!m->stream || !SDL_ResumeAudioStreamDevice(m->stream))return 0;
    }
    return 1;
}
static int worker(void *context) {
    BlazeSdlMusic *m=context;int ok=initialize(m);
    SDL_LockMutex(m->lock);m->ready=ok?1:-1;SDL_SignalCondition(m->wake);SDL_UnlockMutex(m->lock);
    if(ok)for(;;) {
        Request *r;int closing;
        SDL_LockMutex(m->lock);closing=m->closing;r=m->head;
        if(r && !closing) {m->head=r->next;if(!m->head)m->tail=NULL;}
        SDL_UnlockMutex(m->lock);
        if(closing)break;
        if(r) {request(m,r);free(r);continue;}
        if(m->playing && !m->muted) {
            int queued=SDL_GetAudioStreamQueued(m->stream);
            if(queued<0) {publish(m,0,0,1);break;}
            if(m->draining) {
                if(!queued) {m->playing=0;publish(m,0,m->play_sequence,0);}
            } else if(queued<QUEUE_FRAMES*4) {
                int16_t samples[CHUNK*2];fluid_player_t *p=m->slots[m->active];int tick,total;
                if(fluid_synth_write_s16(m->synth,CHUNK,samples,0,2,samples,1,2)!=FLUID_OK ||
                   !SDL_PutAudioStreamData(m->stream,samples,sizeof(samples))) {publish(m,0,0,1);break;}
                tick=fluid_player_get_current_tick(p);total=fluid_player_get_total_ticks(p);
                if(m->loop && tick<m->last_tick) {++m->loops;++m->starts;publish(m,0,0,0);}
                m->last_tick=tick;
                /* Match the SMF end, rather than FluidSynth's additional voice
                   release/reverb grace period. Publish EOF only after queued PCM. */
                if(!m->loop && ((total>0 && tick>=total) || fluid_player_get_status(p)==FLUID_PLAYER_DONE)) {
                    fluid_player_stop(p);m->draining=1;SDL_FlushAudioStream(m->stream);
                }
                continue;
            }
        }
        SDL_LockMutex(m->lock);
        if(!m->head && !m->closing)SDL_WaitConditionTimeout(m->wake,m->lock,4);
        SDL_UnlockMutex(m->lock);
    }
    if(m->synth)stop_song(m);
    if(m->stream)SDL_DestroyAudioStream(m->stream);
    if(m->slots[0])delete_fluid_player(m->slots[0]);
    if(m->slots[1])delete_fluid_player(m->slots[1]);
    if(m->synth)delete_fluid_synth(m->synth);
    if(m->settings)delete_fluid_settings(m->settings);
    return 0;
}
BlazeSdlMusic *blaze_sdl_music_open(const char *root,const char *bank,int muted) {
    BlazeSdlMusic *m=calloc(1,sizeof(*m));if(!m)return NULL;
    m->muted=muted;m->active=m->intent_slot=-1;
    if(strlen(root)>=sizeof(m->root) || strlen(bank)>=sizeof(m->bank)) {free(m);return NULL;}
    strcpy(m->root,root);strcpy(m->bank,bank);
    m->lock=SDL_CreateMutex();m->wake=SDL_CreateCondition();
    if(m->lock && m->wake)m->thread=SDL_CreateThread(worker,"blaze-midi",m);
    if(m->thread) {
        SDL_LockMutex(m->lock);while(!m->ready)SDL_WaitCondition(m->wake,m->lock);SDL_UnlockMutex(m->lock);
        if(m->ready==1)return m;
    }
    blaze_sdl_music_close(m);return NULL;
}
static void submit(BlazeSdlMusic *m,Request *r) {
    if(!m) {free(r);return;}
    SDL_LockMutex(m->lock);
    if(!r) {m->status.failed=1;SDL_UnlockMutex(m->lock);return;}
    r->sequence=++m->status.submitted;
    if(r->operation==PLAY || r->operation==STOP || (r->operation==LOAD && (int)r->slot==m->intent_slot)) {
        m->intent=r->sequence;m->intent_slot=r->operation==PLAY?(int)r->slot:-1;
    }
    if(m->tail)m->tail->next=r;else m->head=r;m->tail=r;
    SDL_SignalCondition(m->wake);SDL_UnlockMutex(m->lock);
}
void blaze_sdl_music_load(BlazeSdlMusic *m,unsigned slot,const char *file,unsigned track,int cd) {
    Request *r;char relative[100];(void)track;if(!m || slot>1)return;
    r=calloc(1,sizeof(*r));if(r) {
        r->operation=LOAD;r->slot=slot;r->cd=cd;
        snprintf(relative,sizeof(relative),"midi/%.79s",file);
        blaze_path(r->path,sizeof(r->path),m->root,relative);
    }submit(m,r);
}
void blaze_sdl_music_play(BlazeSdlMusic *m,unsigned slot,int loop) {
    Request *r;if(slot>1)return;r=calloc(1,sizeof(*r));
    if(r) {r->operation=PLAY;r->slot=slot;r->loop=loop;}submit(m,r);
}
void blaze_sdl_music_stop(BlazeSdlMusic *m) {
    Request *r=calloc(1,sizeof(*r));if(r)r->operation=STOP;submit(m,r);
}
void blaze_sdl_music_status(BlazeSdlMusic *m,BlazeMusicStatus *s) {
    memset(s,0,sizeof(*s));if(!m)return;
    SDL_LockMutex(m->lock);*s=m->status;s->finished=m->finished && m->finished==m->intent;SDL_UnlockMutex(m->lock);
}
int blaze_sdl_music_ended(BlazeSdlMusic *m) {BlazeMusicStatus s;blaze_sdl_music_status(m,&s);return s.finished;}
void blaze_sdl_music_close(BlazeSdlMusic *m) {
    Request *r;if(!m)return;
    if(m->thread) {
        SDL_LockMutex(m->lock);m->closing=1;SDL_SignalCondition(m->wake);SDL_UnlockMutex(m->lock);
        SDL_WaitThread(m->thread,NULL);
    }
    while((r=m->head)!=NULL) {m->head=r->next;free(r);}
    if(m->wake)SDL_DestroyCondition(m->wake);
    if(m->lock)SDL_DestroyMutex(m->lock);free(m);
}
