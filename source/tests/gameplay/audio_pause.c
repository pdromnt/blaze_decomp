#include "blaze.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"audio/pause line %d: %s\n",__LINE__,#x);goto done;}} while(0)
typedef struct {unsigned sounds,loads,plays,stops,silences,slot,track;int cd,finished;char file[80];} Events;
static void sound(void *p,unsigned id) {Events *e=p;if(id<BLAZE_SOUNDS)++e->sounds;}
static void load(void *p,unsigned slot,const char *file,unsigned track,int cd) {Events *e=p;++e->loads;e->slot=slot;e->track=track;e->cd=cd;snprintf(e->file,80,"%s",file);}
static void play(void *p,unsigned slot,int loop) {Events *e=p;(void)loop;++e->plays;e->slot=slot;}
static void stop(void *p) {Events *e=p;++e->stops;e->finished=0;}
static int ended(void *p) {return ((Events *)p)->finished;}
static void silence(void *p) {++((Events *)p)->silences;}
static int press(BlazeApp *a,unsigned input) {return blaze_app_tick(a,0) && blaze_app_tick(a,input);}
int main(int argc,char **argv) {
    BlazeApp *a=calloc(1,sizeof(*a));BlazeWorld *snapshot=malloc(sizeof(*snapshot));
    BlazeSoundBank bank={0};BlazeMixer mix={0};BlazePcm pcm={0};Events e={0};
    BlazeMedia media={&e,sound,load,play,stop,silence,ended};
    int16_t output[32];char error[256];unsigned i,cycle;int result=1;
    /* Stereo signed16 averaging, unsigned8 conversion and transactional rejection. */
    uint8_t wav[]={ 'R','I','F','F',40,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
        1,0,2,0,0x22,0x56,0,0,0x88,0x58,1,0,4,0,16,0,'d','a','t','a',4,0,0,0,0,0x80,0xff,0x7f };
    CHECK(argc==3 && a && snapshot);
    CHECK(blaze_pcm_decode(&pcm,wav,sizeof(wav)) && pcm.count==1 && pcm.samples[0]==0);
    wav[4]=41;CHECK(!blaze_pcm_decode(&pcm,wav,sizeof(wav)) && pcm.count==1);wav[4]=40;
    wav[22]=1;wav[32]=1;wav[34]=8;wav[44]=0;wav[45]=128;wav[46]=255;wav[47]=64;
    CHECK(blaze_pcm_decode(&pcm,wav,sizeof(wav)) && pcm.samples[0]==-32768 && pcm.samples[1]==0 && pcm.samples[2]==32512);
    CHECK(blaze_sound_bank_open(&bank,argv[1],error,sizeof(error)));
    for(i=0;i<BLAZE_SOUNDS;++i) {
        size_t n;int audible=0;CHECK(bank.sounds[i].count && bank.sounds[i].samples);
        for(n=0;n<bank.sounds[i].count;++n)audible|=bank.sounds[i].samples[n]!=0;
        CHECK(audible);
    }
    { /* Two overlapping full-volume voices saturate without signed overflow. */
        int16_t values[]={20000,-20000};BlazeSoundBank clipping={0};BlazeMixer test={0};
        clipping.sounds[0].samples=values;clipping.sounds[0].count=2;clipping.sounds[0].rate=22050;
        test.bank=&clipping;blaze_mixer_play(&test,0);blaze_mixer_play(&test,0);blaze_mixer_mix(&test,output,3);
        CHECK(output[0]==32767 && output[1]==-32768 && !output[2] && !test.voices[0][0].active);
    }
    CHECK(bank.sounds[BLAZE_LIG].rate==11025 && bank.sounds[BLAZE_PI].rate==11025);
    mix.bank=&bank;blaze_mixer_play(&mix,BLAZE_LIG);blaze_mixer_mix(&mix,output,4);
    CHECK(output[0]==bank.sounds[BLAZE_LIG].samples[0] && output[1]==output[0] && output[2]==bank.sounds[BLAZE_LIG].samples[1]);
    for(i=0;i<5;++i)blaze_mixer_play(&mix,BLAZE_SHT);
    CHECK(mix.next[BLAZE_SHT]==5);for(i=0;i<4;++i)CHECK(mix.voices[BLAZE_SHT][i].active && !mix.voices[BLAZE_SHT][i].position);
    CHECK(blaze_app_open(a,argv[1],argv[2],NULL));blaze_app_set_media(a,&media);
    CHECK(!strcmp(a->tracks[3][0].file,"blz_optf.mid")); /* First duplicate Option1 wins. */
    for(i=0;i<6;++i) {
        BlazeRandom expected=a->random;a->music=(int)i;
        if(i%3==0)blaze_random_next(&expected);
        blaze_music_load(a,7,0);CHECK(a->random.state==expected.state && e.cd==(i>=3));
        CHECK(a->music_choice[0]<3 && !strcmp(e.file,a->tracks[7][a->music_choice[0]].file));
    }
    /* The rare inclusive random endpoint selects the original missing third key. */
    a->music=0;for(i=0;i<200000;++i) {BlazeRandom expected={0};expected.state=i;if(blaze_random_next(&expected)==32767)break;}
    CHECK(i<200000);a->random.state=i;blaze_music_load(a,7,0);CHECK(a->music_choice[0]==2 && !strcmp(e.file,"0"));
    for(cycle=0;cycle<3;++cycle) {
        if(cycle==0)for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<180;++i)CHECK(blaze_app_tick(a,i%4==0?BLAZE_CONFIRM:0));
        CHECK(a->state==BLAZE_GAME && a->world.active && e.loads && e.plays && e.sounds);
        CHECK(press(a,BLAZE_PAUSE));CHECK(a->pause_flags==2);*snapshot=a->world;
        blaze_mixer_play(&mix,BLAZE_HIT);
        {uint32_t rng=a->random.state;unsigned tick=a->tick,sounds=e.sounds,loads=e.loads,plays=e.plays;
            for(i=0;i<120;++i)CHECK(blaze_app_tick(a,BLAZE_CONFIRM|BLAZE_DOWN));
            CHECK(e.sounds==sounds && e.loads==loads && e.plays==plays);
            blaze_mixer_mix(&mix,output,32);CHECK(mix.voices[BLAZE_HIT][0].position>0);
            CHECK(!memcmp(snapshot,&a->world,sizeof(*snapshot)) && rng==a->random.state && tick==a->tick);
        }
        blaze_app_pause(a,1,1);CHECK(press(a,BLAZE_PAUSE));CHECK(a->pause_flags==1);
        blaze_app_pause(a,1,0);CHECK(!a->pause_flags);
        CHECK(blaze_app_tick(a,0));CHECK(a->world.tick==snapshot->tick+1);
        CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_MAIN));
        CHECK(a->state==BLAZE_TITLE && !a->pause_flags && !a->world.active && !a->level_data.objects.bytes && e.silences);
    }
    /* Music edits are deferred: preview replays the loaded track, cancel discards. */
    for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));CHECK(press(a,BLAZE_DOWN));CHECK(press(a,BLAZE_DOWN));CHECK(press(a,BLAZE_CONFIRM));
    CHECK(a->state==BLAZE_OPTIONS);CHECK(blaze_app_tick(a,0));
    CHECK(press(a,BLAZE_DOWN));CHECK(press(a,BLAZE_DOWN));CHECK(press(a,BLAZE_RIGHT));
    CHECK(a->music==0 && a->music_pending==1);
    {unsigned loads=e.loads,plays=e.plays;uint32_t rng=a->random.state;
        CHECK(press(a,BLAZE_CONFIRM));CHECK(e.loads==loads && e.plays==plays+1 && rng==a->random.state);
    }
    CHECK(press(a,BLAZE_CANCEL));CHECK(a->state==BLAZE_TITLE && a->music==0);
    for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));CHECK(press(a,BLAZE_CONFIRM));CHECK(a->state==BLAZE_OPTIONS);
    CHECK(blaze_app_tick(a,0));CHECK(press(a,BLAZE_RIGHT));CHECK(a->music_pending==1);
    CHECK(press(a,BLAZE_DOWN));CHECK(press(a,BLAZE_DOWN));CHECK(press(a,BLAZE_CONFIRM));
    CHECK(a->state==BLAZE_TITLE && a->music==1);
    CHECK(blaze_app_tick(a,0));e.finished=1;CHECK(blaze_app_tick(a,0));
    CHECK(a->state==BLAZE_RANKING);
    CHECK(press(a,BLAZE_MAIN));CHECK(a->state==BLAZE_TITLE);
    CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_QUIT));CHECK(a->state==BLAZE_EXIT);
    result=0;puts("PCM decoding/resampling, four-voice overlap, music RNG/config, pause/focus and repeated paused combat teardown passed");
done:
    if(a)blaze_app_close(a);free(a);free(snapshot);free(pcm.samples);blaze_sound_bank_close(&bank);return result;
}
