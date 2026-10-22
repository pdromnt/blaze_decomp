#include "blaze.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Explicit RIFF/PCM decoding: no casts to host WAVE headers or sample pointers. */
int blaze_pcm_decode(BlazePcm *out,const uint8_t *b,size_t size) {
    size_t end,offset=12,frames,i;unsigned channels=0,bits=0,rate=0,align=0;
    const uint8_t *data=NULL;size_t length=0;int16_t *samples;
    if(size<12 || memcmp(b,"RIFF",4) || memcmp(b+8,"WAVE",4))return 0;
    end=(size_t)blaze_le32(b+4)+8;if(end>size || end<12)return 0;
    while(offset+8<=end) {
        uint32_t n=blaze_le32(b+offset+4);const uint8_t *p=b+offset+8;
        if(n>end-offset-8)return 0;
        if(!memcmp(b+offset,"fmt ",4)) {
            if(n<16 || blaze_le16(p)!=1)return 0;
            channels=blaze_le16(p+2);rate=blaze_le32(p+4);align=blaze_le16(p+12);bits=blaze_le16(p+14);
        } else if(!memcmp(b+offset,"data",4)) {data=p;length=n;}
        offset+=8+(size_t)n+(n&1);
    }
    if(!data || !length || (channels!=1 && channels!=2) || (bits!=8 && bits!=16) ||
       !rate || rate>192000 || align!=channels*(bits/8) || length%align)return 0;
    frames=length/align;if(frames>SIZE_MAX/sizeof(*samples))return 0;
    samples=malloc(frames*sizeof(*samples));if(!samples)return 0;
    for(i=0;i<frames;++i) {
        int value=0;unsigned c;
        for(c=0;c<channels;++c) {
            const uint8_t *p=data+i*align+c*(bits/8);
            value+=bits==8?((int)*p-128)*256:(blaze_le16(p)<32768?(int)blaze_le16(p):(int)blaze_le16(p)-65536);
        }
        samples[i]=(int16_t)(value/(int)channels);
    }
    free(out->samples);out->samples=samples;out->count=frames;out->rate=rate;return 1;
}
void blaze_sound_bank_close(BlazeSoundBank *bank) {
    unsigned i;for(i=0;i<BLAZE_SOUNDS;++i)free(bank->sounds[i].samples);memset(bank,0,sizeof(*bank));
}
int blaze_sound_bank_open(BlazeSoundBank *bank,const char *root,char *error,size_t capacity) {
    static const char *files[]={"shot","hit","kill","suu","kan","pi","ligh","back","ded","uee","goo","bosu","sun","puu","pun","get","gb","bal"};
    BlazeSoundBank next={0};unsigned i;
    for(i=0;i<BLAZE_SOUNDS;++i) {
        char name[64],path[1200];uint8_t *bytes=NULL;size_t size=0;int ok;
        snprintf(name,sizeof(name),"se/se_%s.wav",files[i]);
        ok=blaze_path(path,sizeof(path),root,name) && blaze_file_read(path,&bytes,&size) && blaze_pcm_decode(&next.sounds[i],bytes,size);
        free(bytes);
        if(!ok) {snprintf(error,capacity,"Missing/invalid PCM effect: %s",name);blaze_sound_bank_close(&next);return 0;}
    }
    blaze_sound_bank_close(bank);*bank=next;return 1;
}
void blaze_mixer_play(BlazeMixer *m,unsigned sound) {
    BlazeVoice *v;if(sound>=BLAZE_SOUNDS || !m->bank)return;
    /* Original DirectSound rotates four duplicates, restarting the chosen voice. */
    v=&m->voices[sound][m->next[sound]++%4];v->position=0;v->active=1;
}
void blaze_mixer_mix(BlazeMixer *m,int16_t *output,size_t count) {
    size_t n;unsigned i,j;
    if(!m->bank) {memset(output,0,count*sizeof(*output));return;}
    for(n=0;n<count;++n) {
        int sum=0;
        for(i=0;i<BLAZE_SOUNDS;++i)for(j=0;j<4;++j) {
            BlazeVoice *v=&m->voices[i][j];const BlazePcm *s=&m->bank->sounds[i];size_t index;
            if(!v->active)continue;index=(size_t)(v->position>>32);
            if(index>=s->count) {v->active=0;continue;}
            sum+=s->samples[index];v->position+=((uint64_t)s->rate<<32)/22050;
            if((v->position>>32)>=s->count)v->active=0;
        }
        output[n]=(int16_t)(sum>32767?32767:sum< -32768?-32768:sum);
    }
}
