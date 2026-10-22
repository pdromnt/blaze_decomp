#include "blaze.h"

static void translate(uint32_t *s) {s[5]+=s[9];s[7]+=s[9];s[6]+=s[10];s[8]+=s[10];}
static void center(uint32_t *s,const BlazeSprite *sprite,const uint32_t *rect,int64_t mx,int64_t my) {
    s[5]=rect[0]+(uint32_t)(mx+blaze_signed(rect[2]-rect[0])/2-sprite->width/2);
    s[6]=rect[1]+(uint32_t)(my+blaze_signed(rect[3]-rect[1])/2-sprite->visible_height/2);
    s[7]=s[5]+sprite->width;s[8]=s[6]+sprite->visible_height;
}
static void resize(BlazeWorld *w,uint32_t *s,unsigned sprite) {
    const BlazeSprite *old=blaze_level_sprite(w->level,s[0]);
    const BlazeSprite *next=blaze_level_sprite(w->level,sprite);
    s[5]+=(uint32_t)(((int)old->width-next->width)/2);
    s[6]+=(uint32_t)(((int)old->visible_height-next->visible_height)/2);
    s[7]=s[5]+next->width;s[8]=s[6]+next->visible_height;s[0]=sprite;s[12]=(uint32_t)-2;
}
/* 0x419a20 tests the target's active flag, not the source's. This matters when
   one shot is queried against both actor categories after its first impact. */
int blaze_records_overlap(const uint32_t *a,const uint32_t *b) {
    int64_t ae=blaze_signed(a[12]),be=blaze_signed(b[12]);
    return b[4] && (int64_t)blaze_signed(a[5])-ae<(int64_t)blaze_signed(b[7])+be &&
        (int64_t)blaze_signed(b[5])-be<(int64_t)blaze_signed(a[7])+ae &&
        (int64_t)blaze_signed(a[6])-ae<(int64_t)blaze_signed(b[8])+be &&
        (int64_t)blaze_signed(b[6])-be<(int64_t)blaze_signed(a[8])+ae;
}
int blaze_primary_create(BlazeWorld *w,const int32_t trig[320],uint32_t *s,int angle_offset) {
    const BlazeSprite *sprite;unsigned direction,ship=w->loadout->ship;
    if(s[4])return 0;
    sprite=blaze_level_sprite(w->level,s[0]);
    center(s,sprite,w->player.words+5,(int64_t)w->loadout->weapons[0].mount_x[ship]+2,
        (int64_t)w->loadout->weapons[0].mount_y[ship]+4);
    s[3]=0;s[4]=1;s[11]=w->player.words[11]+(uint32_t)angle_offset;direction=s[11]&255;
    s[9]=(uint32_t)(trig[direction]/12);s[10]=(uint32_t)(-trig[direction+64]/12);
    return 1;
}
/* Action 10: scan from the beginning for each ray. A full pool refuses a ray
   and keeps live records intact, exactly as 0x410db0/0x4144c0 do. */
void blaze_primary_fire(BlazeWorld *w,const int32_t trig[320]) {
    int first=w->loadout->ship==0?-6:w->loadout->ship==1?-2:-32;
    int step=w->loadout->ship==0?6:w->loadout->ship==1?2:8;
    int last=w->loadout->ship==2?0:-first,angle;unsigned i;
    blaze_sound(w,BLAZE_SHT);
    for(angle=first;angle<=last;angle+=step)for(i=0;i<48;++i)
        if(blaze_primary_create(w,trig,w->player.words+(0xb20+i*0x34)/4,angle))break;
}
static int rotated(BlazeWorld *w,const uint32_t *s,unsigned frame,int scale) {
    const BlazeSprite *sprite=blaze_level_sprite(w->level,s[0]);BlazeRotatedDraw *draw;
    if(blaze_signed(s[5])>=w->camera_x+320 || blaze_signed(s[7])<=w->camera_x ||
        blaze_signed(s[6])>=w->camera_y+240 || blaze_signed(s[8])<=w->camera_y)return 1;
    if(w->shot_draw_count==sizeof(w->shot_draws)/sizeof(w->shot_draws[0]))return 0;
    draw=&w->shot_draws[w->shot_draw_count++];
    draw->sprite=s[0]+frame;draw->angle=s[11]&255;draw->scale=scale;
    draw->pivot_x=sprite->width/2;draw->pivot_y=sprite->visible_height/2;
    draw->x=blaze_signed(s[5])-w->camera_x+draw->pivot_x;
    draw->y=blaze_signed(s[6])-w->camera_y-4+draw->pivot_y;
    return 1;
}
int blaze_weapon_hit(BlazeWorld *w,const uint32_t *shot,unsigned damage,int special) {
    unsigned i;int hit=0;
    for(i=0;i<256;++i) {
        uint32_t *actor=w->actors[i];
        if((special?(actor[0x44/4]&128)!=0:(actor[0x44/4]&2)==0) && blaze_records_overlap(shot,actor)) {
            actor[0x50/4]-=damage;
            if(blaze_signed(actor[0x64/4])<(int)damage)actor[0x64/4]=damage;
            hit=1;
        }
    }
    return hit;
}
/* 0x411530/0x415430. Each slot consumes the selection roll even when busy;
   selected busy slots also consume the direction roll before refusing reuse. */
void blaze_weapon_impact(BlazeWorld *w,const int32_t trig[320],const uint32_t *rect,unsigned kind) {
    unsigned i;
    for(i=0;i<32;++i)if(blaze_random_next(w->random)*7/32767==0) {
        unsigned direction=blaze_random_next(w->random)*256/32767;
        uint32_t *s=w->player.words+(0x37bc+i*0x34)/4;
        if(s[4])continue;
        center(s,blaze_level_sprite(w->level,s[0]),rect,0,0);
        s[11]=direction;s[9]=(uint32_t)(trig[direction&255]/8);
        s[10]=(uint32_t)(-trig[(direction&255)+64]/8);s[3]=s[4]=1;
        resize(w,s,0x61+kind);
    }
}
int blaze_primary_update(BlazeWorld *w,const int32_t trig[320],uint32_t *s) {
    int first,second;unsigned pulse;
    if(!s[4])return 1;
    translate(s);first=blaze_weapon_hit(w,s,4,0);
    if(first) {
        s[4]=0;blaze_weapon_impact(w,trig,s+5,2);
        if(!rotated(w,s,3,256))return 0;
    }
    second=blaze_weapon_hit(w,s,5,1);
    if(second) {
        s[4]=0;blaze_weapon_impact(w,trig,s+5,2);
        if(!rotated(w,s,3,256))return 0;
    } else if(!first) {
        pulse=s[3]%6;pulse=pulse<3?pulse:5-pulse;
        if(!rotated(w,s,pulse,256))return 0;
    }
    if(++s[3]>32)s[4]=0;
    return 1;
}
int blaze_sparks_update(BlazeWorld *w) {
    unsigned i;
    for(i=0;i<32;++i) {
        uint32_t *s=w->player.words+(0x37bc+i*0x34)/4;unsigned ry,rx;
        if(!s[4])continue;
        if(!rotated(w,s,0,256+(int)s[3]*48))return 0;
        ry=blaze_random_next(w->random);rx=blaze_random_next(w->random);
        s[10]=(uint32_t)(blaze_signed(s[10])/(int)(ry*3/32767+1));
        s[9]=(uint32_t)(blaze_signed(s[9])/(int)(rx*3/32767+1));
        translate(s);if(s[3]++>8)s[4]=0;
    }
    return 1;
}
