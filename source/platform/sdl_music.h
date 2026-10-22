#ifndef BLAZE_SDL_MUSIC_H
#define BLAZE_SDL_MUSIC_H
#include "music_status.h"
typedef struct BlazeSdlMusic BlazeSdlMusic;
BlazeSdlMusic *blaze_sdl_music_open(const char *root,const char *bank,int muted);
void blaze_sdl_music_load(BlazeSdlMusic *,unsigned slot,const char *file,unsigned track,int cd);
void blaze_sdl_music_play(BlazeSdlMusic *,unsigned slot,int loop);
void blaze_sdl_music_stop(BlazeSdlMusic *);
int blaze_sdl_music_ended(BlazeSdlMusic *);
void blaze_sdl_music_status(BlazeSdlMusic *,BlazeMusicStatus *);
void blaze_sdl_music_close(BlazeSdlMusic *);
#endif
