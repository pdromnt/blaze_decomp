/* Real callback/stream adapter, then dummy or silent real-device transport. */
#define SDL_MAIN_HANDLED
#include "../../platform/sdl_pcm.c"
#include <SDL3/SDL_main.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"SDL PCM line %d: %s (%s)\n",__LINE__,#x,SDL_GetError());goto done;}} while(0)
int main(int argc,char **argv) {
    BlazeSdlPcm *p=calloc(1,sizeof(*p));SDL_AudioSpec spec={SDL_AUDIO_S16,1,22050};
    int result=1,initialized=0,device=argc==3 && !strcmp(argv[2],"--device");unsigned i;
    CHECK(argc==3 && p);
    SDL_SetMainReady();
    if(!device)CHECK(SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy"));
    if(!SDL_Init(SDL_INIT_AUDIO)) {
        if(device) {puts("No SDL audio driver available; real-device acceptance unavailable");result=77;goto done;}
        CHECK(0);
    }
    initialized=1;
    printf("SDL PCM driver: %s\n",SDL_GetCurrentAudioDriver());
    if(device && (!strcmp(SDL_GetCurrentAudioDriver(),"dummy") || !strcmp(SDL_GetCurrentAudioDriver(),"disk"))) {
        puts("A non-hardware SDL driver is selected; real-device acceptance unavailable");result=77;goto done;
    }
    if(!strcmp(argv[2],"--stream")) {
        int16_t expected[700],actual[700];BlazeMixer reference;
        CHECK(blaze_sdl_pcm_open(p,argv[1],1) && !p->stream);
        p->stream=SDL_CreateAudioStream(&spec,&spec);CHECK(p->stream);
        CHECK(SDL_SetAudioStreamGetCallback(p->stream,pcm_feed,p));
        for(i=0;i<5;++i)blaze_sdl_pcm_sound(p,BLAZE_SHT);
        blaze_sdl_pcm_sound(p,BLAZE_LIG);blaze_sdl_pcm_sound(p,BLAZE_PI);
        reference=p->mixer;blaze_mixer_mix(&reference,expected,700);
        CHECK(SDL_GetAudioStreamData(p->stream,actual,sizeof(actual))==sizeof(actual));
        CHECK(!memcmp(expected,actual,sizeof(actual)) && p->mixer.next[BLAZE_SHT]==5);
        CHECK(!memcmp(reference.voices,p->mixer.voices,sizeof(reference.voices)));
        CHECK(blaze_sdl_pcm_submitted(p)==700 && !blaze_sdl_pcm_failed(p));
        CHECK(SDL_PutAudioStreamData(p->stream,expected,sizeof(expected)));
        CHECK(SDL_GetAudioStreamQueued(p->stream)==sizeof(expected));
        blaze_sdl_pcm_silence(p);CHECK(SDL_GetAudioStreamQueued(p->stream)==0);
        for(i=0;i<BLAZE_SOUNDS;++i) {unsigned j;for(j=0;j<4;++j)CHECK(!p->mixer.voices[i][j].active);}
        memset(actual,1,sizeof(actual));CHECK(SDL_GetAudioStreamData(p->stream,actual,sizeof(actual))==sizeof(actual));
        for(i=0;i<700;++i)CHECK(!actual[i]);
        blaze_sdl_pcm_close(p);
        /* Failed open remains safely closable, without a device or bank leak. */
        CHECK(!blaze_sdl_pcm_open(p,"/blaze-test-missing-assets",0));blaze_sdl_pcm_close(p);
        result=0;puts("SDL callback matches mixer samples/voices, clears queued sound, resets and closes failed owners");goto done;
    }
    CHECK(!strcmp(argv[2],"--dummy") || device);
    if(device) {
        int count=0;SDL_AudioDeviceID *ids=SDL_GetAudioPlaybackDevices(&count);SDL_free(ids);
        if(!count) {puts("No SDL playback device; real-device acceptance unavailable");result=77;goto done;}
    }
    for(i=0;i<6;++i) {
        uint64_t before,after;unsigned j;
        CHECK(blaze_sdl_pcm_open(p,argv[1],0) && p->stream);
        /* Only this app's stream gain changes. The host device always receives
           silence, even while real SFX voices advance below. */
        CHECK(SDL_SetAudioStreamGain(p->stream,0));blaze_sdl_pcm_sound(p,BLAZE_SHT);
        before=blaze_sdl_pcm_submitted(p);
        SDL_Delay(400); /* No main-thread pump: expose the old frame-starvation bug. */
        after=blaze_sdl_pcm_submitted(p);
        CHECK(after-before>=6500 && !blaze_sdl_pcm_failed(p));
        for(j=0;j<20;++j) {blaze_sdl_pcm_sound(p,BLAZE_SHT);blaze_sdl_pcm_silence(p);}
        CHECK(!blaze_sdl_pcm_failed(p));blaze_sdl_pcm_close(p);
        CHECK(!p->stream && !p->bank.sounds[0].samples && !p->initialized);
    }
    result=0;puts("SDL independent PCM during frame stalls, silence/reset and six active close/reopen cycles passed");
done:
    if(p)blaze_sdl_pcm_close(p);free(p);if(initialized)SDL_Quit();return result;
}
