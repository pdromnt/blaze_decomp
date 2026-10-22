#include "blaze.h"
#include <string.h>

static void center(BlazeWorld *w,uint32_t *s,int x,int y) {
    const BlazeSprite *sprite=blaze_level_sprite(w->level,s[0]);const uint32_t *p=w->player.words;
    s[5]=p[5]+(uint32_t)((int64_t)x+blaze_signed(p[7]-p[5])/2-sprite->width/2);
    s[6]=p[6]+(uint32_t)((int64_t)y+blaze_signed(p[8]-p[6])/2-sprite->visible_height/2);
    s[7]=s[5]+sprite->width;s[8]=s[6]+sprite->visible_height;
}
static int submit(BlazeWorld *w,const uint32_t *s,unsigned frame,int scale,int blend) {
    const BlazeSprite *sprite=blaze_level_sprite(w->level,s[0]);BlazeActorDraw *d;
    if(w->effect_draw_count>=sizeof(w->effect_draws)/sizeof(w->effect_draws[0]))return 0;
    d=&w->effect_draws[w->effect_draw_count++];memset(d,0,sizeof(*d));d->sprite=s[0]+frame;d->scale=scale;d->palette=-1;d->blend=blend;
    d->x=blaze_signed(s[5])-w->camera_x+((int)sprite->width-(int)((int64_t)sprite->width*scale/256))/2;
    d->y=blaze_signed(s[6])-w->camera_y+((int)sprite->visible_height-(int)((int64_t)sprite->visible_height*scale/256))/2;
    return 1;
}
int blaze_player_contact(BlazeWorld *w,const uint32_t *actor,unsigned damage) {
    uint32_t *p=w->player.words;unsigned i;
    if(!blaze_records_overlap(actor,p))return 0;
    if(w->loadout->ship==2 && p[0x4238/4]) {blaze_sound(w,BLAZE_KAN);return 1;}
    p[0x2ad0/4]-=damage;p[0x2ad4/4]=0u-damage;p[0x2aa8/4]=0;
    if(blaze_signed(p[0x2ad0/4])<0)p[0x2ad0/4]=0;
    if(damage) {
        for(i=0;i<20;++i) {
            uint32_t *s=p+(0x1888+i*0x40)/4;
            if(s[4])continue;
            s[3]=16-damage/4;s[4]=1;s[0x34/4]=1; /* Player handle. */
            s[0x38/4]=p[5]-actor[5];s[0x3c/4]=p[6]-actor[6];blaze_sound(w,BLAZE_HIT);break;
        }
        w->scroll_speed=-2;
    }
    return 1;
}
int blaze_damage_effects(BlazeWorld *w) {
    unsigned i;
    for(i=0;i<20;++i) {
        uint32_t *s=w->player.words+(0x1888+i*0x40)/4;
        if(!s[4])continue;
        if(s[3]&1) {
            int dy,dx;center(w,s,0,0);
            dy=(int)(blaze_random_next(w->random)*16/32767)-8;
            dx=(int)(blaze_random_next(w->random)*16/32767)-8;
            s[5]+=(uint32_t)dx;s[7]+=(uint32_t)dx;s[6]+=(uint32_t)dy;s[8]+=(uint32_t)dy;
            if(!submit(w,s,(s[3]/4)%4,(40-(int)s[3])*16,0))return 0;
        }
        if(++s[3]>23)s[4]=0;
    }
    return 1;
}
void blaze_player_regenerate(BlazeWorld *w) {
    uint32_t *p=w->player.words;unsigned i,interval=w->loadout->ship==0?8:w->loadout->ship==1?12:4;
    if(blaze_signed(p[0xc4/4])<34 || p[0x2ad0/4]>=p[0x2ae4/4] || w->loadout->mode==2)return;
    if(blaze_random_next(w->random)*2/32767)for(i=0;i<8;++i) {
        uint32_t *s=p+(0x1d88+i*0x34)/4;int dy,dx;
        if(s[4])continue;
        s[4]=1;s[3]=0;
        dy=(int)(blaze_random_next(w->random)*32/32767)-16;
        dx=(int)(blaze_random_next(w->random)*32/32767);
        center(w,s,dx,dy);break;
    }
    if(w->tick%interval==0)++p[0x2ad0/4];
}
int blaze_player_present_effects(BlazeWorld *w) {
    uint32_t *p=w->player.words,*trail=p+0x2ae8/4;unsigned i;
    for(i=0;i<8;++i) {
        uint32_t *s=p+(0x1d88+i*0x34)/4;
        if(!s[4])continue;
        if(!submit(w,s,s[3]/2,256,1))return 0;
        if(++s[3]>7)s[4]=0;
    }
    if(p[0x2ad0/4]>=48) {trail[4]=0;return 1;}
    if(!trail[4]) {
        trail[0x34/4]=1;trail[0x38/4]=trail[0x3c/4]=trail[3]=0;trail[4]=1;
        center(w,trail,0,0);
        for(i=0;i<6;++i) {
            memcpy(trail+0x40/4+i*4,trail+5,16);
            trail[0xa0/4+i]=blaze_random_next(w->random)*2/32767;
        }
    }
    memmove(trail+0x40/4,trail+0x50/4,5*16);
    memmove(trail+0xa0/4,trail+0xa4/4,5*4);
    center(w,trail,0,0);memcpy(trail+0x90/4,trail+5,16);
    trail[0xb4/4]=blaze_random_next(w->random)*2/32767;
    for(i=0;i<6;++i)if(!trail[0xa0/4+i]) {
        memcpy(trail+5,trail+0x40/4+i*4,16);
        if(!submit(w,trail,0,256-(int)i*32,1))return 0;
    }
    return 1;
}
