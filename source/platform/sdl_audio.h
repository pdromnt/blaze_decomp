#ifndef BLAZE_SDL_AUDIO_H
#define BLAZE_SDL_AUDIO_H
#include "sdl_pcm.h"
#include "windows_music.h"
/* SDL owns PCM; the existing independent Windows MIDI worker remains explicit.
   There is no WaveOut owner or second sound bank in the SDL application. */
typedef struct {
    BlazeSdlPcm pcm;BlazeWindowsMusic *music;BlazeMedia media;char root[1024];
} BlazeSdlAudio;
int blaze_sdl_audio_open(BlazeSdlAudio *audio,const char *root,int muted);
void blaze_sdl_audio_pump(void *audio);
void blaze_sdl_audio_close(BlazeSdlAudio *audio);
#endif
