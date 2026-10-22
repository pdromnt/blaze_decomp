#ifndef BLAZE_WINDOWS_AUDIO_H
#define BLAZE_WINDOWS_AUDIO_H
#include <windows.h>
#include <mmsystem.h>
#include "blaze.h"
#include "windows_music.h"
typedef struct {
    BlazeSoundBank bank;BlazeMixer mixer;BlazeMedia media;
    HWAVEOUT output;WAVEHDR headers[4];int16_t samples[4][256];
    /* Immutable bank; one mixer guarded only while playing/clearing/mixing.
       The PCM worker alone submits buffers. Driver calls never hold this lock. */
    CRITICAL_SECTION mixer_lock;
    HANDLE pcm_wake,pcm_thread;
    int lock_ready;
    volatile LONG pcm_closing,pcm_reset,pcm_submitted;
    BlazeWindowsMusic *music;
    char root[1024];int muted;volatile LONG output_failed;
} BlazeWindowsAudio;
/* Open a fresh owner; close even after failure, after the app has detached. */
int blaze_windows_audio_open(BlazeWindowsAudio *audio,const char *root,int muted);
void blaze_windows_audio_pump(BlazeWindowsAudio *audio);
void blaze_windows_audio_close(BlazeWindowsAudio *audio);
#endif
