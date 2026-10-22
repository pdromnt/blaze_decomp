#include "sdl_pcm.h"
#include <stdio.h>
#include <string.h>

/* SDL holds the stream lock during this callback. Only the immutable bank and
   mixer/transport owner are visible here; never game state, I/O or music calls. */
static void SDLCALL pcm_feed(void *context,SDL_AudioStream *stream,int additional,int total) {
    BlazeSdlPcm *p=context;int16_t samples[256];size_t remaining;
    (void)total;
    if(additional<=0 || SDL_GetAtomicInt(&p->failed))return;
    remaining=((size_t)(unsigned)additional+sizeof(int16_t)-1)/sizeof(int16_t);
    while(remaining) {
        size_t count=remaining>256?256:remaining;
        blaze_mixer_mix(&p->mixer,samples,count);
        if(!SDL_PutAudioStreamData(stream,samples,(int)(count*sizeof(int16_t)))) {
            SDL_SetAtomicInt(&p->failed,1);return;
        }
        p->submitted_samples+=count;remaining-=count;
    }
}
int blaze_sdl_pcm_open(BlazeSdlPcm *p,const char *root,int muted) {
    SDL_AudioSpec spec={SDL_AUDIO_S16,1,22050};char error[256];
    memset(p,0,sizeof(*p));p->muted=muted;
    if(!blaze_sound_bank_open(&p->bank,root,error,sizeof(error))) {fprintf(stderr,"SDL PCM: %s\n",error);return 0;}
    p->mixer.bank=&p->bank;
    if(muted)return 1;
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO))return 0;p->initialized=1;
    p->stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,pcm_feed,p);
    if(!p->stream || !SDL_ResumeAudioStreamDevice(p->stream)) {
        fprintf(stderr,"SDL PCM device: %s\n",SDL_GetError());return 0;
    }
    return 1;
}
void blaze_sdl_pcm_sound(BlazeSdlPcm *p,unsigned id) {
    if(p->stream && !SDL_LockAudioStream(p->stream)) {SDL_SetAtomicInt(&p->failed,1);return;}
    blaze_mixer_play(&p->mixer,id);
    if(p->stream)SDL_UnlockAudioStream(p->stream);
}
void blaze_sdl_pcm_silence(BlazeSdlPcm *p) {
    if(p->stream && !SDL_LockAudioStream(p->stream)) {SDL_SetAtomicInt(&p->failed,1);return;}
    memset(p->mixer.voices,0,sizeof(p->mixer.voices));
    /* Stream-only operations are safe under its recursive lock. Never call a
       device-locking API while locked: SDL's callback takes device then stream. */
    if(p->stream) {
        if(!SDL_ClearAudioStream(p->stream))SDL_SetAtomicInt(&p->failed,1);
        SDL_UnlockAudioStream(p->stream);
    }
}
void blaze_sdl_pcm_pump(BlazeSdlPcm *p) {
    if(p->muted) {
        int16_t scratch[256];blaze_mixer_mix(&p->mixer,scratch,256);
    }
}
int blaze_sdl_pcm_failed(BlazeSdlPcm *p) {return SDL_GetAtomicInt(&p->failed)!=0;}
uint64_t blaze_sdl_pcm_submitted(BlazeSdlPcm *p) {
    uint64_t count;
    if(!p->stream)return p->submitted_samples;
    if(!SDL_LockAudioStream(p->stream)) {SDL_SetAtomicInt(&p->failed,1);return 0;}
    count=p->submitted_samples;SDL_UnlockAudioStream(p->stream);return count;
}
void blaze_sdl_pcm_close(BlazeSdlPcm *p) {
    /* Destruction waits for any callback and closes the convenience device.
       Callback-visible mixer/bank storage must remain alive until it returns. */
    if(p->stream)SDL_DestroyAudioStream(p->stream);
    if(p->initialized)SDL_QuitSubSystem(SDL_INIT_AUDIO);
    blaze_sound_bank_close(&p->bank);memset(p,0,sizeof(*p));
}
