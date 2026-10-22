/* Test this owner directly: its synth/stream are accessed only before starting
   the worker, or by the worker itself. No desktop input or audible output. */
#include "../../platform/sdl_music.c"
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"SDL MIDI line %d: %s (%s)\n",__LINE__,#x,SDL_GetError());goto done;}} while(0)
static BlazeMusicStatus state(BlazeSdlMusic *m) {BlazeMusicStatus s;blaze_sdl_music_status(m,&s);return s;}
static int settled(BlazeSdlMusic *m) {
    unsigned i;for(i=0;i<2000;++i) {BlazeMusicStatus s=state(m);if(s.completed==s.submitted)return !s.failed;SDL_Delay(1);}return 0;
}
static int wait_end(BlazeSdlMusic *m) {
    unsigned i;for(i=0;i<2000;++i) {if(blaze_sdl_music_ended(m))return 1;SDL_Delay(1);}return 0;
}
static int write_midi(const char *path) {
    /* Format 1, two tracks, 96 PPQ: 96 ticks at 500000 us, then 96
       at 250000 us. Total 750 ms; note releases leave an audible tail. */
    static const uint8_t bytes[]={
        'M','T','h','d',0,0,0,6,0,1,0,2,0,96,
        'M','T','r','k',0,0,0,18,
        0,255,81,3,7,161,32,96,255,81,3,3,208,144,96,255,47,0,
        'M','T','r','k',0,0,0,36,
        0,192,80,0,176,1,64,0,176,5,32,0,176,65,127,
        0,176,91,75,0,176,93,45,0,144,60,100,
        129,64,128,60,0,0,255,47,0};
    FILE *f=fopen(path,"wb");int ok;if(!f)return 0;
    ok=fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes);return fclose(f)==0 && ok;
}
typedef struct {fluid_synth_t *synth;unsigned count;int failed;} Corpus;
static SDL_EnumerationResult corpus_file(void *context,const char *dir,const char *name) {
    Corpus *c=context;char path[1200];size_t length=strlen(name);fluid_player_t *p;int16_t pcm[1024];
    if(length<4 || SDL_strcasecmp(name+length-4,".mid"))return SDL_ENUM_CONTINUE;
    if(!blaze_path(path,sizeof(path),dir,name)) {c->failed=1;return SDL_ENUM_FAILURE;}
    p=new_fluid_player(c->synth);
    if(!p) {c->failed=1;return SDL_ENUM_FAILURE;}
    fluid_synth_system_reset(c->synth);
    if(fluid_player_add(p,path)!=FLUID_OK || fluid_player_play(p)!=FLUID_OK ||
       fluid_synth_write_s16(c->synth,512,pcm,0,2,pcm,1,2)!=FLUID_OK || fluid_player_get_status(p)!=FLUID_PLAYER_PLAYING) {
        fprintf(stderr,"MIDI corpus load failed: %s\n",path);c->failed=1;
    }
    delete_fluid_player(p);++c->count;return c->failed?SDL_ENUM_FAILURE:SDL_ENUM_CONTINUE;
}
int main(int argc,char **argv) {
    BlazeSdlMusic *m=NULL;BlazeSdlMusic offline={0};char bank[1200],dir[1200],file[1200];
    fluid_player_t *p=NULL;int result=1,i,cc;int16_t samples[512*2];uint64_t energy=0;
    CHECK(argc==3);CHECK(SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy"));CHECK(SDL_Init(SDL_INIT_AUDIO));
    CHECK(blaze_path(bank,sizeof(bank),SDL_GetBasePath(),"resources/GeneralUser-GS.sf2"));
    CHECK(blaze_path(dir,sizeof(dir),argv[1],"midi"));CHECK(SDL_CreateDirectory(dir));
    CHECK(blaze_path(file,sizeof(file),dir,"timing.mid"));CHECK(write_midi(file));
    /* Dependency ambiguity: tempo and the controllers actually used by BLAZE
       must reach synthesis, including modulation/portamento/effect sends. */
    offline.muted=1;strcpy(offline.bank,bank);CHECK(initialize(&offline));
    {Corpus corpus={offline.synth,0,0};char midi[1200];
    CHECK(blaze_path(midi,sizeof(midi),argv[2],"midi"));
    CHECK(SDL_EnumerateDirectory(midi,corpus_file,&corpus));CHECK(!corpus.failed && corpus.count==32);}
    { /* All three SysEx payloads found in the shipped MIDI corpus, without F7.
         SC-88 MODE-1 selects a single hardware module/port; our one-port synth
         already has that topology. GM-on and GS-reset must really reset CCs. */
    static const char gm[]={0x7e,0x7f,9,1};
    static const char gs[]={0x41,0x10,0x42,0x12,0x40,0,0x7f,0,0x41};
    static const char mode[]={0x41,0x10,0x42,0x12,0,0,0x7f,0,1};int handled;
    CHECK(fluid_synth_sysex(offline.synth,mode,sizeof(mode),NULL,NULL,&handled,0)==FLUID_OK && !handled);
    CHECK(fluid_synth_cc(offline.synth,0,1,65)==FLUID_OK);
    CHECK(fluid_synth_sysex(offline.synth,gm,sizeof(gm),NULL,NULL,&handled,0)==FLUID_OK && handled);
    CHECK(fluid_synth_get_cc(offline.synth,0,1,&cc)==FLUID_OK && cc==0);
    CHECK(fluid_synth_cc(offline.synth,0,1,65)==FLUID_OK);
    CHECK(fluid_synth_sysex(offline.synth,gs,sizeof(gs),NULL,NULL,&handled,0)==FLUID_OK && handled);
    CHECK(fluid_synth_get_cc(offline.synth,0,1,&cc)==FLUID_OK && cc==0);}
    p=new_fluid_player(offline.synth);CHECK(p);CHECK(fluid_player_add(p,file)==FLUID_OK);CHECK(fluid_player_play(p)==FLUID_OK);
    for(i=0;i<60;++i) {
        int n;CHECK(fluid_synth_write_s16(offline.synth,512,samples,0,2,samples,1,2)==FLUID_OK);
        for(n=0;n<1024;++n)energy+=(uint64_t)(samples[n]<0?-(int)samples[n]:samples[n]);
    }
    CHECK(energy>100000);CHECK(fluid_player_get_current_tick(p)>160 && fluid_player_get_current_tick(p)<182);
    CHECK(fluid_player_get_total_ticks(p)==192);
    CHECK(fluid_synth_get_cc(offline.synth,0,1,&cc)==FLUID_OK && cc==64);
    CHECK(fluid_synth_get_cc(offline.synth,0,5,&cc)==FLUID_OK && cc==32);
    CHECK(fluid_synth_get_cc(offline.synth,0,65,&cc)==FLUID_OK && cc==127);
    CHECK(fluid_synth_get_cc(offline.synth,0,91,&cc)==FLUID_OK && cc==75);
    CHECK(fluid_synth_get_cc(offline.synth,0,93,&cc)==FLUID_OK && cc==45);
    delete_fluid_player(p);p=NULL;
    /* A stopped higher-index slot's deferred mute must not kill a new song in
       an older player. Exercise both sample-timer registration orders. */
    offline.active=-1;offline.lock=SDL_CreateMutex();CHECK(offline.lock);
    {Request r={0};r.operation=LOAD;strcpy(r.path,file);request(&offline,&r);r.slot=1;request(&offline,&r);
    for(i=0;i<4;++i) {
        int n;r.operation=PLAY;r.slot=(unsigned)(1-(i&1));request(&offline,&r);
        for(n=0;n<20;++n)CHECK(fluid_synth_write_s16(offline.synth,512,samples,0,2,samples,1,2)==FLUID_OK);
        CHECK(fluid_synth_get_active_voice_count(offline.synth)>0);
    }}
    stop_song(&offline);delete_fluid_player(offline.slots[0]);delete_fluid_player(offline.slots[1]);
    offline.slots[0]=offline.slots[1]=NULL;SDL_DestroyMutex(offline.lock);offline.lock=NULL;
    delete_fluid_synth(offline.synth);offline.synth=NULL;
    delete_fluid_settings(offline.settings);offline.settings=NULL;
    m=blaze_sdl_music_open(argv[1],bank,0);CHECK(m);
    /* Zero only this stream before any notes. Never touch system volume. */
    CHECK(SDL_SetAudioStreamGain(m->stream,0));
    for(i=0;i<3;++i) {
        uint64_t loops=state(m).loops,begin;
        blaze_sdl_music_load(m,0,"timing.mid",0,0);blaze_sdl_music_load(m,1,"timing.mid",0,0);
        blaze_sdl_music_play(m,0,1);CHECK(settled(m));CHECK(state(m).valid[0] && state(m).valid[1]);
        if(!i) {
            unsigned n;for(n=0;n<1500 && state(m).loops==loops;++n)SDL_Delay(1);
            CHECK(state(m).loops>loops && !blaze_sdl_music_ended(m));
        }
        begin=SDL_GetTicks();blaze_sdl_music_play(m,1,0);CHECK(!blaze_sdl_music_ended(m));CHECK(settled(m));
        CHECK(wait_end(m));CHECK(SDL_GetTicks()-begin>=700 && SDL_GetTicks()-begin<1300);
        CHECK(!state(m).playing);
        blaze_sdl_music_load(m,0,"timing.mid",0,0);CHECK(settled(m));CHECK(blaze_sdl_music_ended(m));
        blaze_sdl_music_play(m,0,1);CHECK(!blaze_sdl_music_ended(m));CHECK(settled(m));
        blaze_sdl_music_stop(m);CHECK(settled(m));CHECK(!state(m).playing && !blaze_sdl_music_ended(m));
        CHECK(SDL_GetAudioStreamQueued(m->stream)==0);
    }
    /* Close with commands pending; reopen repeatedly with one bank owner. */
    for(i=0;i<12;++i) {blaze_sdl_music_load(m,i&1,"timing.mid",0,0);blaze_sdl_music_play(m,i&1,1);}
    blaze_sdl_music_close(m);m=NULL;
    for(i=0;i<4;++i) {m=blaze_sdl_music_open(argv[1],bank,1);CHECK(m);blaze_sdl_music_load(m,0,"timing.mid",0,0);blaze_sdl_music_play(m,0,1);blaze_sdl_music_close(m);m=NULL;}
    CHECK(!blaze_sdl_music_open(argv[1],"missing-bank.sf2",1));
    puts("MIDI tempo/controllers/synthesis, two-slot EOF/loops, FIFO/stop/drain and pending-close/reopen passed");result=0;
done:
    blaze_sdl_music_close(m);
    if(p)delete_fluid_player(p);
    if(offline.slots[0])delete_fluid_player(offline.slots[0]);
    if(offline.slots[1])delete_fluid_player(offline.slots[1]);
    if(offline.lock)SDL_DestroyMutex(offline.lock);
    if(offline.synth)delete_fluid_synth(offline.synth);
    if(offline.settings)delete_fluid_settings(offline.settings);
    SDL_Quit();return result;
}
