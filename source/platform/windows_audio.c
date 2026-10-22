#define _CRT_SECURE_NO_WARNINGS
#include "windows_audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

static void sound(void *context,unsigned id) {
    BlazeWindowsAudio *a=context;
    EnterCriticalSection(&a->mixer_lock);blaze_mixer_play(&a->mixer,id);LeaveCriticalSection(&a->mixer_lock);
}
static void silence(void *context) {
    BlazeWindowsAudio *a=context;
    EnterCriticalSection(&a->mixer_lock);
    memset(a->mixer.voices,0,sizeof(a->mixer.voices));
    LeaveCriticalSection(&a->mixer_lock);
    if(a->pcm_wake) {InterlockedExchange(&a->pcm_reset,1);SetEvent(a->pcm_wake);}
}
static unsigned __stdcall pcm_worker(void *context) {
    BlazeWindowsAudio *a=context;unsigned i;
    for(;;) {
        if(InterlockedCompareExchange(&a->pcm_closing,0,0))break;
        if(InterlockedExchange(&a->pcm_reset,0))waveOutReset(a->output);
        for(i=0;i<4;++i) {
            WAVEHDR *h=&a->headers[i];
            if(!(h->dwFlags&WHDR_INQUEUE)) {
                EnterCriticalSection(&a->mixer_lock);
                blaze_mixer_mix(&a->mixer,a->samples[i],256);
                LeaveCriticalSection(&a->mixer_lock);
                if(waveOutWrite(a->output,h,sizeof(*h))) {
                    fprintf(stderr,"Audio: waveOutWrite failed; output stopped\n");
                    InterlockedExchange(&a->output_failed,1);waveOutReset(a->output);return 0;
                }
                InterlockedIncrement(&a->pcm_submitted);
            }
        }
        WaitForSingleObject(a->pcm_wake,INFINITE);
    }
    return 0;
}
/* Main-thread callbacks only submit native music requests. The music owner
   performs file I/O, MCI calls and looping exclusively on its worker. */
static void stop(void *context) {BlazeWindowsAudio *a=context;blaze_windows_music_stop(a->music);}
static void load(void *context,unsigned slot,const char *file,unsigned track,int cd) {BlazeWindowsAudio *a=context;blaze_windows_music_load(a->music,slot,file,track,cd);}
static void play(void *context,unsigned slot,int loop) {BlazeWindowsAudio *a=context;blaze_windows_music_play(a->music,slot,loop);}
static int ended(void *context) {BlazeWindowsAudio *a=context;return blaze_windows_music_ended(a->music);}
int blaze_windows_audio_open(BlazeWindowsAudio *a,const char *root,int muted) {
    WAVEFORMATEX format={0};char error[256];unsigned i;MMRESULT result;
    memset(a,0,sizeof(*a));a->muted=muted;
    InitializeCriticalSection(&a->mixer_lock);a->lock_ready=1;
    if(strlen(root)>=sizeof(a->root))return 0;strcpy(a->root,root);
    if(!blaze_sound_bank_open(&a->bank,root,error,sizeof(error))) {fprintf(stderr,"Audio: %s\n",error);return 0;}
    a->mixer.bank=&a->bank;a->media.context=a;a->media.sound=sound;a->media.load=load;
    a->media.play=play;a->media.stop=stop;a->media.silence=silence;a->media.ended=ended;
    a->music=blaze_windows_music_open(a->root,muted);
    if(!a->music) {fprintf(stderr,"Audio: cannot create music worker\n");return 0;}
    if(muted)return 1;
    format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=1;format.nSamplesPerSec=22050;
    format.wBitsPerSample=16;format.nBlockAlign=2;format.nAvgBytesPerSec=44100;
    a->pcm_wake=CreateEventA(NULL,FALSE,FALSE,NULL);if(!a->pcm_wake)return 0;
    result=waveOutOpen(&a->output,WAVE_MAPPER,&format,(DWORD_PTR)a->pcm_wake,0,CALLBACK_EVENT);
    if(result) {char text[256];waveOutGetErrorTextA(result,text,sizeof(text));fprintf(stderr,"Audio output unavailable: %s\n",text);a->output=NULL;}
    if(a->output)for(i=0;i<4;++i) {
        a->headers[i].lpData=(LPSTR)a->samples[i];a->headers[i].dwBufferLength=sizeof(a->samples[i]);
        if(waveOutPrepareHeader(a->output,&a->headers[i],sizeof(WAVEHDR))) {fprintf(stderr,"Audio: cannot prepare PCM buffer\n");return 0;}
    }
    if(a->output) {
        a->pcm_thread=(HANDLE)_beginthreadex(NULL,0,pcm_worker,a,0,NULL);
        if(!a->pcm_thread) {fprintf(stderr,"Audio: cannot create PCM worker\n");return 0;}
    }
    return 1;
}
void blaze_windows_audio_pump(BlazeWindowsAudio *a) {
    if(a->muted) {int16_t scratch[256];blaze_mixer_mix(&a->mixer,scratch,256);return;}
    /* Real PCM completion is serviced independently of rendering and menus. */
}
void blaze_windows_audio_close(BlazeWindowsAudio *a) {
    unsigned i;
    if(a->pcm_thread) {
        InterlockedExchange(&a->pcm_closing,1);SetEvent(a->pcm_wake);
        WaitForSingleObject(a->pcm_thread,INFINITE);CloseHandle(a->pcm_thread);
    }
    blaze_windows_music_close(a->music);
    if(a->output) {
        waveOutReset(a->output);
        for(i=0;i<4;++i)if(a->headers[i].dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(a->output,&a->headers[i],sizeof(WAVEHDR));
        waveOutClose(a->output);
    }
    if(a->pcm_wake)CloseHandle(a->pcm_wake);
    if(a->lock_ready)DeleteCriticalSection(&a->mixer_lock);
    blaze_sound_bank_close(&a->bank);memset(a,0,sizeof(*a));
}
