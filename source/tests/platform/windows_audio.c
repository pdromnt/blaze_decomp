/* Real device transport with silent PCM/MIDI only; no window or desktop input. */
#include "windows_audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"Windows audio line %d: %s\n",__LINE__,#x);goto done;}} while(0)
static double call_max,pump_max;static LARGE_INTEGER frequency;
#define MEASURE(call,maximum) do {LARGE_INTEGER begin,end;double ms;QueryPerformanceCounter(&begin);call;QueryPerformanceCounter(&end);ms=(double)(end.QuadPart-begin.QuadPart)*1000/frequency.QuadPart;if(ms>maximum)maximum=ms;} while(0)
static BlazeMusicStatus state(BlazeWindowsAudio *a) {BlazeMusicStatus s;blaze_windows_music_status(a->music,&s);return s;}
/* These waits belong only to this diagnostic. The application never waits for
   music completion, opens, stops or a command acknowledgement. */
static int settled(BlazeWindowsAudio *a) {
    unsigned n;for(n=0;n<1000;++n) {BlazeMusicStatus s=state(a);if(s.completed==s.submitted)return !s.failed;MEASURE(blaze_windows_audio_pump(a),pump_max);Sleep(5);}return 0;
}
static int looped(BlazeWindowsAudio *a,uint64_t previous) {
    unsigned n;for(n=0;n<800;++n) {if(state(a).loops>previous)return 1;MEASURE(blaze_windows_audio_pump(a),pump_max);Sleep(5);}return 0;
}
static int ended(BlazeWindowsAudio *a) {
    unsigned n;for(n=0;n<800;++n) {if(a->media.ended(a))return 1;MEASURE(blaze_windows_audio_pump(a),pump_max);Sleep(5);}return 0;
}
int main(int argc,char **argv) {
    /* Same two-second fixture as the synchronous baseline; no note-on,
       controller or sysex events. Terminal note-off establishes MCI duration. */
    static const unsigned char midi[]={ 'M','T','h','d',0,0,0,6,0,0,0,1,0,96,
        'M','T','r','k',0,0,0,16,0,255,81,3,7,161,32,131,0,143,0,0,0,255,47,0 };
    BlazeWindowsAudio *a=calloc(1,sizeof(*a));char temp[MAX_PATH],root[MAX_PATH],dir[MAX_PATH],file[MAX_PATH];
    FILE *f=NULL;int result=1;unsigned i;DWORD handles_before,handles_after;root[0]=dir[0]=file[0]=0;
    QueryPerformanceFrequency(&frequency);CHECK(argc==2 && a);
    CHECK(blaze_windows_audio_open(a,argv[1],1));CHECK(!a->output);
    a->media.load(a,0,"blz_sg1f.mid",1,0);a->media.play(a,0,1);CHECK(settled(a));
    CHECK(state(a).valid[0] && state(a).playing);
    a->media.sound(a,BLAZE_SHT);CHECK(a->mixer.voices[BLAZE_SHT][0].active);
    blaze_windows_audio_pump(a);CHECK(a->mixer.voices[BLAZE_SHT][0].position>0);
    a->media.silence(a);CHECK(!a->mixer.voices[BLAZE_SHT][0].active);blaze_windows_audio_close(a);
    /* Queue cancellation/join must release both worker handles and requests. */
    CHECK(GetProcessHandleCount(GetCurrentProcess(),&handles_before));
    for(i=0;i<20;++i) {
        unsigned n;CHECK(blaze_windows_audio_open(a,argv[1],1));
        for(n=0;n<16;++n) {a->media.load(a,n&1,"blz_sg1f.mid",1,0);a->media.play(a,n&1,1);a->media.stop(a);}
        blaze_windows_audio_close(a);
    }
    CHECK(GetProcessHandleCount(GetCurrentProcess(),&handles_after));CHECK(handles_after==handles_before);
    if(!waveOutGetNumDevs() || !midiOutGetNumDevs()) {puts("No host audio/MIDI device; silent device acceptance unavailable");result=77;goto done;}
    CHECK(GetTempPathA(sizeof(temp),temp));snprintf(root,sizeof(root),"%sblaze-audio-%lu",temp,GetCurrentProcessId());
    CHECK(CreateDirectoryA(root,NULL));snprintf(dir,sizeof(dir),"%s/midi",root);CHECK(CreateDirectoryA(dir,NULL));
    snprintf(file,sizeof(file),"%s/silent.mid",dir);f=fopen(file,"wb");CHECK(f);
    CHECK(fwrite(midi,1,sizeof(midi),f)==sizeof(midi));fclose(f);f=NULL;
    CHECK(blaze_windows_audio_open(a,argv[1],0));CHECK(a->output);strcpy(a->root,root);
    /* Regression: the game used to refill only every 40 ms, starving its
       46 ms PCM queue. Completions must now progress with no main-thread pump,
       including a render stall. The mixer is silent throughout this check. */
    {
        LONG before,after;MMTIME position={0};DWORD start,end;
        for(i=0;i<100 && InterlockedCompareExchange(&a->pcm_submitted,0,0)<4;++i)Sleep(5);
        CHECK(InterlockedCompareExchange(&a->pcm_submitted,0,0)>=4);
        before=InterlockedCompareExchange(&a->pcm_submitted,0,0);
        position.wType=TIME_SAMPLES;CHECK(!waveOutGetPosition(a->output,&position,sizeof(position)));
        CHECK(position.wType==TIME_SAMPLES);start=position.u.sample;
        Sleep(400);
        after=InterlockedCompareExchange(&a->pcm_submitted,0,0);
        position.wType=TIME_SAMPLES;CHECK(!waveOutGetPosition(a->output,&position,sizeof(position)));
        end=position.u.sample;
        CHECK(after-before>=24 && end-start>=6500 && !a->output_failed);
    }
    for(i=0;i<3;++i) {
        BlazeMusicStatus s=state(a);uint64_t loops=s.loops;
        MEASURE(a->media.load(a,0,"silent.mid",1,0),call_max);
        MEASURE(a->media.load(a,1,"silent.mid",1,0),call_max);
        MEASURE(a->media.play(a,0,1),call_max);
        MEASURE(blaze_windows_audio_pump(a),pump_max);
        CHECK(settled(a));s=state(a);CHECK(s.valid[0] && s.valid[1] && s.playing && s.active==0);
        if(i==0) {CHECK(looped(a,loops));CHECK(state(a).playing && !a->media.ended(a) && !a->output_failed);}
        MEASURE(a->media.play(a,1,0),call_max);CHECK(!a->media.ended(a));CHECK(settled(a));
        s=state(a);CHECK(s.playing && s.active==1);
        if(i==0) {
            CHECK(ended(a));CHECK(!state(a).playing);
            /* Preloading an inactive slot must not hide the active song's EOF. */
            MEASURE(a->media.load(a,0,"silent.mid",1,0),call_max);CHECK(settled(a));CHECK(a->media.ended(a));
            MEASURE(a->media.play(a,0,1),call_max);CHECK(!a->media.ended(a));CHECK(settled(a));CHECK(state(a).playing);
        }
        a->media.silence(a);MEASURE(a->media.stop(a),call_max);CHECK(!a->media.ended(a));CHECK(settled(a));CHECK(!state(a).playing);
    }
    /* Fast scene changes preserve FIFO ordering without waiting for the driver. */
    MEASURE(a->media.load(a,0,"silent.mid",1,0),call_max);
    MEASURE(a->media.play(a,0,1),call_max);MEASURE(a->media.stop(a),call_max);
    MEASURE(a->media.load(a,1,"silent.mid",1,0),call_max);MEASURE(a->media.play(a,1,0),call_max);
    CHECK(settled(a));CHECK(state(a).playing && state(a).active==1 && !a->media.ended(a));
    /* Exit during a fresh real-device transition, without draining its queue. */
    MEASURE(a->media.load(a,0,"silent.mid",1,0),call_max);MEASURE(a->media.play(a,0,1),call_max);
    MEASURE(a->media.load(a,1,"silent.mid",1,0),call_max);MEASURE(a->media.play(a,1,0),call_max);
    blaze_windows_audio_close(a);
    /* Warm the PCM-only reopen path after MCI shutdown before sampling driver
       caches. Observed first-reopen growth was two handles, then flat counts;
       every subsequent close must return exactly to this warmed baseline. */
    CHECK(blaze_windows_audio_open(a,argv[1],0));Sleep(60);blaze_windows_audio_close(a);
    CHECK(GetProcessHandleCount(GetCurrentProcess(),&handles_before));
    for(i=0;i<3;++i) {
        CHECK(blaze_windows_audio_open(a,argv[1],0));CHECK(a->output && a->pcm_thread);
        Sleep(60);a->media.silence(a);blaze_windows_audio_close(a);
        CHECK(GetProcessHandleCount(GetCurrentProcess(),&handles_after));CHECK(handles_after==handles_before);
    }
    CHECK(GetProcessHandleCount(GetCurrentProcess(),&handles_after));CHECK(handles_after==handles_before);
    printf("Main-thread music calls max %.3f ms; frame audio pump max %.3f ms\n",call_max,pump_max);
    result=0;puts("Worker MIDI ordering/loop/EOF, pending-close cleanup and independent silent PCM transport/stall/reset ownership passed");
done:
    if(f)fclose(f);if(a)blaze_windows_audio_close(a);free(a);
    if(file[0])DeleteFileA(file);if(dir[0])RemoveDirectoryA(dir);if(root[0])RemoveDirectoryA(root);return result;
}
