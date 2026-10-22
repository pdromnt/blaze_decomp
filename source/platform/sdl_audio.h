#ifndef BLAZE_SDL_AUDIO_H
#define BLAZE_SDL_AUDIO_H
#include "sdl_pcm.h"
#include "sdl_music.h"
/* Independent SDL PCM and MIDI owners, joined before their shared root dies. */
typedef struct {
    BlazeSdlPcm pcm;BlazeSdlMusic *music;BlazeMedia media;char root[1024];
} BlazeSdlAudio;
int blaze_sdl_audio_open(BlazeSdlAudio *audio,const char *root,int muted);
void blaze_sdl_audio_pump(void *audio);
void blaze_sdl_audio_close(BlazeSdlAudio *audio);
#endif
