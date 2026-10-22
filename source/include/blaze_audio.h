#ifndef BLAZE_AUDIO_H
#define BLAZE_AUDIO_H
#include <stddef.h>
#include <stdint.h>

enum { BLAZE_SHT,BLAZE_HIT,BLAZE_KIL,BLAZE_SUU,BLAZE_KAN,BLAZE_PI,
       BLAZE_LIG,BLAZE_BACK,BLAZE_DED,BLAZE_UEE,BLAZE_GOO,BLAZE_BOSU,
       BLAZE_SUN,BLAZE_PUU,BLAZE_PUN,BLAZE_GET,BLAZE_GB,BLAZE_BAL,BLAZE_SOUNDS };
typedef struct {int16_t *samples;size_t count;unsigned rate;} BlazePcm;
typedef struct {BlazePcm sounds[BLAZE_SOUNDS];} BlazeSoundBank;
typedef struct {uint64_t position;int active;} BlazeVoice;
typedef struct {
    const BlazeSoundBank *bank;
    BlazeVoice voices[BLAZE_SOUNDS][4];unsigned next[BLAZE_SOUNDS];
} BlazeMixer;
/* Non-owning native platform callbacks. The platform owns devices and sample bank. */
typedef struct {
    void *context;
    void (*sound)(void *,unsigned);
    void (*load)(void *,unsigned,const char *,unsigned,int);
    void (*play)(void *,unsigned,int);
    void (*stop)(void *);
    void (*silence)(void *);
    int (*ended)(void *); /* Non-looping music reached its real device end. */
} BlazeMedia;
/* PCM/bank owners must start zeroed; failed replacements retain the old data. */
int blaze_pcm_decode(BlazePcm *out,const uint8_t *bytes,size_t size);
int blaze_sound_bank_open(BlazeSoundBank *bank,const char *root,char *error,size_t capacity);
void blaze_sound_bank_close(BlazeSoundBank *bank);
void blaze_mixer_play(BlazeMixer *mixer,unsigned sound);
void blaze_mixer_mix(BlazeMixer *mixer,int16_t *output,size_t count);
#endif
