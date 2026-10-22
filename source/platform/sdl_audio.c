#include "sdl_audio.h"
#include <stdio.h>
#include <string.h>
static void sound(void *context,unsigned id) {BlazeSdlAudio *a=context;blaze_sdl_pcm_sound(&a->pcm,id);}
static void silence(void *context) {BlazeSdlAudio *a=context;blaze_sdl_pcm_silence(&a->pcm);}
static void stop(void *context) {BlazeSdlAudio *a=context;blaze_windows_music_stop(a->music);}
static void load(void *context,unsigned slot,const char *file,unsigned track,int cd) {BlazeSdlAudio *a=context;blaze_windows_music_load(a->music,slot,file,track,cd);}
static void play(void *context,unsigned slot,int loop) {BlazeSdlAudio *a=context;blaze_windows_music_play(a->music,slot,loop);}
static int ended(void *context) {BlazeSdlAudio *a=context;return blaze_windows_music_ended(a->music);}
int blaze_sdl_audio_open(BlazeSdlAudio *a,const char *root,int muted) {
    memset(a,0,sizeof(*a));
    if(strlen(root)>=sizeof(a->root))return 0;strcpy(a->root,root);
    if(!blaze_sdl_pcm_open(&a->pcm,root,muted))return 0;
    a->music=blaze_windows_music_open(a->root,muted);
    if(!a->music) {fprintf(stderr,"SDL audio: cannot create MIDI worker\n");return 0;}
    a->media.context=a;a->media.sound=sound;a->media.silence=silence;
    a->media.load=load;a->media.play=play;a->media.stop=stop;a->media.ended=ended;
    return 1;
}
void blaze_sdl_audio_pump(void *context) {BlazeSdlAudio *a=context;blaze_sdl_pcm_pump(&a->pcm);}
void blaze_sdl_audio_close(BlazeSdlAudio *a) {
    blaze_windows_music_close(a->music);blaze_sdl_pcm_close(&a->pcm);memset(a,0,sizeof(*a));
}
