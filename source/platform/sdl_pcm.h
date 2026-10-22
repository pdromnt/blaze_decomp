#ifndef BLAZE_SDL_PCM_H
#define BLAZE_SDL_PCM_H
#include <SDL3/SDL.h>
#include "blaze.h"
typedef struct {
    BlazeSoundBank bank;BlazeMixer mixer;
    SDL_AudioStream *stream;
    SDL_AtomicInt failed;
    uint64_t submitted_samples; /* Read only while holding the stream lock. */
    int initialized,muted;
} BlazeSdlPcm;
/* Fresh zeroed owner; close is also valid after a failed open. */
int blaze_sdl_pcm_open(BlazeSdlPcm *pcm,const char *root,int muted);
void blaze_sdl_pcm_sound(BlazeSdlPcm *pcm,unsigned id);
void blaze_sdl_pcm_silence(BlazeSdlPcm *pcm);
void blaze_sdl_pcm_pump(BlazeSdlPcm *pcm);
int blaze_sdl_pcm_failed(BlazeSdlPcm *pcm);
uint64_t blaze_sdl_pcm_submitted(BlazeSdlPcm *pcm);
void blaze_sdl_pcm_close(BlazeSdlPcm *pcm);
#endif
