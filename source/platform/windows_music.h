#ifndef BLAZE_WINDOWS_MUSIC_H
#define BLAZE_WINDOWS_MUSIC_H
#include "music_status.h"
typedef struct BlazeWindowsMusic BlazeWindowsMusic;
/* Read-only publication, never a second writable playback owner. */
/* root is borrowed from the audio owner. Only the submitting thread reads it;
   queued requests own their complete paths. Close joins before root is freed. */
BlazeWindowsMusic *blaze_windows_music_open(const char *root,int muted);
void blaze_windows_music_load(BlazeWindowsMusic *music,unsigned slot,const char *file,unsigned track,int cd);
void blaze_windows_music_play(BlazeWindowsMusic *music,unsigned slot,int loop);
void blaze_windows_music_stop(BlazeWindowsMusic *music);
int blaze_windows_music_ended(BlazeWindowsMusic *music);
void blaze_windows_music_status(BlazeWindowsMusic *music,BlazeMusicStatus *status);
void blaze_windows_music_close(BlazeWindowsMusic *music);
#endif
