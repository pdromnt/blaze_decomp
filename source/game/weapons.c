#include "blaze.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* 0x407740 owns ordering; all effects remain in the player's one allocation.
   The X history extends from 0x4228 through 0x42d3, before the next record. */
static void move(uint32_t *s) {s[5]+=s[9];s[7]+=s[9];s[6]+=s[10];s[8]+=s[10];}
static void center(BlazeWorld *w,uint32_t *s,const uint32_t *r,int x,int y) {
    const BlazeSprite *sp=blaze_level_sprite(w->level,s[0]);
    s[5]=r[0]+(uint32_t)(x+blaze_signed(r[2]-r[0])/2-sp->width/2);
    s[6]=r[1]+(uint32_t)(y+blaze_signed(r[3]-r[1])/2-sp->visible_height/2);
    s[7]=s[5]+sp->width;s[8]=s[6]+sp->visible_height;
}
static void resize(BlazeWorld *w,uint32_t *s,unsigned id) {
    const BlazeSprite *old=blaze_level_sprite(w->level,s[0]),*sp=blaze_level_sprite(w->level,id);
    s[5]+=(uint32_t)(((int)old->width-sp->width)/2);s[6]+=(uint32_t)(((int)old->visible_height-sp->visible_height)/2);
    s[7]=s[5]+sp->width;s[8]=s[6]+sp->visible_height;s[0]=id;s[12]=(uint32_t)-2;
}
static int draw(BlazeWorld *w,const uint32_t *s,unsigned frame,int blend,int palette) {
    BlazeActorDraw *d;
    if(w->effect_draw_count>=sizeof(w->effect_draws)/sizeof(w->effect_draws[0]))return 0;
    d=&w->effect_draws[w->effect_draw_count++];memset(d,0,sizeof(*d));
    d->sprite=s[0]+frame;d->x=blaze_signed(s[5])-w->camera_x;d->y=blaze_signed(s[6])-w->camera_y;
    d->scale=256;d->palette=palette;d->blend=blend;return 1;
}
static int scaled(BlazeWorld *w,const uint32_t *s,unsigned frame,int scale,int blend) {
    BlazeActorDraw *d;const BlazeSprite *sp=blaze_level_sprite(w->level,s[0]);
    if(!draw(w,s,frame,blend,-1))return 0;
    d=&w->effect_draws[w->effect_draw_count-1];d->scale=scale;
    d->x+=((int)sp->width-(int)sp->width*scale/256)/2;
    d->y+=((int)sp->visible_height-(int)sp->visible_height*scale/256)/2;return 1;
}
static int rotate(BlazeWorld *w,const uint32_t *s,unsigned frame,unsigned angle) {
    BlazeActorDraw *d;const BlazeSprite *sp=blaze_level_sprite(w->level,s[0]);
    if(blaze_signed(s[5])>=w->camera_x+320 || blaze_signed(s[7])<=w->camera_x ||
       blaze_signed(s[6])>=w->camera_y+240 || blaze_signed(s[8])<=w->camera_y)return 1;
    if(!draw(w,s,frame,0,-1))return 0;
    d=&w->effect_draws[w->effect_draw_count-1];d->rotated=1;d->angle=angle&255;
    d->pivot_x=sp->width/2;d->pivot_y=sp->visible_height/2;d->x+=d->pivot_x;d->y+=d->pivot_y-4;return 1;
}
static int energy(BlazeWorld *w,unsigned cost) {
    uint32_t *p=w->player.words;
    if(p[0x2adc/4] || blaze_signed(p[0x68/4])>0)return 0;
    if(w->loadout->mode==2)return 1;
    p[0x2ad8/4]-=cost;
    if(blaze_signed(p[0x2ad8/4])<0) {
        p[0x2ad8/4]=0;p[0x2adc/4]=1;
        if(w->loadout->mode!=1 && w->loadout->mode!=4)blaze_player_weapon(&w->player,w->loadout,0);
    }
    return 1;
}
/* 0x415690 deliberately translates the old rectangle before the next update
   anchors the shot to the current mount. Busy slots refuse all changes. */
static int fire_storm_create(BlazeWorld *w,int offset) {
    uint32_t *p=w->player.words,*s;unsigned i,angle;
    for(i=0;i<10;++i) {
        s=p+(0x2ba0+i*0x34)/4;if(s[4])continue;
        s[11]=p[11]+(uint32_t)offset;angle=s[11]&255;
        s[9]=(uint32_t)(w->trig[angle]/14);s[10]=(uint32_t)(-w->trig[angle+64]/14);
        move(s);s[3]=0;s[4]=1;return 1;
    }
    return 0;
}
/* Shared charge reset (0x411610); flags/targets belong to the same bounded
   0x42d4..0x4367 record, not a second target registry. */
static void charge_clear(uint32_t *s) {
    unsigned i;s[4]=0;
    for(i=0;i<8;++i)s[0x34/4+i]=s[0x74/4+i]=0;
}
static void nova_create(BlazeWorld *w,uint32_t *s,uint32_t side) {
    uint32_t *p=w->player.words;unsigned i;
    s[3]=0;s[4]=1;s[0x34/4]=side;s[11]=p[11];center(w,s,p+0xb4/4,0,0);
    s[0xb8/4]=(uint32_t)(blaze_signed(p[0xc8/4]*2)/3);
    for(i=0;i<8;++i)memcpy(s+0x38/4+i*4,s+5,16);
}
static uint32_t *target(BlazeWorld *w,uint32_t handle) {
    return handle && handle<=256?w->actors[handle-1]:NULL;
}
static const BlazeSprite *target_sprite(BlazeWorld *w,unsigned id) {
    /* Reset registry slots carry persistent font sprite 4 until constructed.
       A retained lock still presents once when it observes that inactive slot. */
    return id<0x40?(id<w->font->count?&w->font->sprites[id]:NULL):blaze_level_sprite(w->level,id);
}
static int thunder_create(BlazeWorld *w,uint32_t *s) {
    if(s[4])return 0;
    s[4]=1;blaze_sound(w,BLAZE_KIL);s[3]=0;w->player.words[0x64/4]=4;w->player.words[0x68/4]=16;return 1;
}
int blaze_weapon_action(BlazeWorld *w,unsigned action,int *accepted,char *error,size_t capacity) {
    uint32_t *p=w->player.words,*s;unsigned i;
    *accepted=0;
    switch(action) {
    case 0:
        s=p+0x2a68/4;
        if(!s[4] && energy(w,16)) {s[3]=0;s[4]=1;*accepted=1;}
        return 1;
    case 1:case 2:case 81:case 82:return 1; /* Original default hold/release cases. */
    case 80:
        s=p+0x3fb4/4;
        if(!s[4] && energy(w,24)) {
            s[3]=0;s[4]=1;s[0x38/4]=0;blaze_sound(w,BLAZE_PUN);resize(w,s,0xa9);
            center(w,s,p+0xb4/4,8,0);s[9]=14;s[10]=0;*accepted=1;
        }
        return 1;
    case 160:case 162:*accepted=1;return 1;
    case 161:
        for(i=0;i<24;++i)if(p[(0x1f28+i*0x78+0x10)/4])return 1;
        blaze_sound(w,BLAZE_SUN);
        for(i=0;i<24;++i) {
            unsigned angle=blaze_random_next(w->random)*256/32767;
            const BlazeSprite *sp;s=p+(0x1f28+i*0x78)/4;
            if(!energy(w,1))continue;
            sp=blaze_level_sprite(w->level,s[0]);s[9]=(uint32_t)(w->trig[angle&255]/48);
            s[10]=(uint32_t)(-w->trig[(angle&255)+64]/48);
            s[5]=p[5]+16;s[6]=p[6]+16;s[7]=s[5]+sp->width;s[8]=s[6]+sp->visible_height;
            s[11]=angle;s[3]=0;s[4]=1;
        }
        return 1;
    case 6:thunder_create(w,p+0x1f0/4);return 1; /* Direct original trigger returns zero. */
    case 8:return 1; /* Direct trigger's release has no action in 0x410db0. */
    case 10:blaze_primary_fire(w,w->trig);return 1;
    case 11:case 12:case 13:
        *accepted=1;if(w->tick%16==0)blaze_sound(w,BLAZE_BAL);
        if(w->tick%16<12)for(i=0;i<32;++i) {
            unsigned direction=blaze_random_next(w->random)*3/32767;
            s=p+(0x4368+i*0x34)/4;
            if(s[4])continue;
            s[3]=0;s[4]=1;s[11]=direction;
            center(w,s,p+5,(int)(blaze_random_next(w->random)*16/32767)-32,-6);break;
        }
        return 1;
    case 30:case 31:
        if(action==31)p[0x68/4]=0;
        if(!energy(w,1))return 1;
        s=p+(action==30?0x1588:0x15bc)/4;s[3]=0;s[4]=1;
        p[0x64/4]=p[0x68/4]=action==30?8:4;*accepted=1;return 1;
    case 32:return 1; /* Original dispatcher has no release action for FORCE JET. */
    case 40:case 41:case 110:case 111:case 120:case 121:case 130:case 131:
        s=p+0x42d4/4;
        if(!s[4] && energy(w,action>=120 && action<130?0:1)) {s[11]=action<100?2:action<120?0:action<130?1:3;s[4]=1;*accepted=1;}
        return 1;
    case 42:
        charge_clear(p+0x42d4/4);
        if(blaze_signed(p[0xc8/4])>10 && energy(w,6)) {
            s=p+0x2da8/4;s[3]=0;s[4]=1;blaze_sound(w,BLAZE_LIG);p[0x64/4]=p[0x68/4]=5;*accepted=1;
        }
        return 1;
    case 50:
        if(energy(w,2))*accepted=fire_storm_create(w,0);
        return 1;
    case 51:
        if(energy(w,1) && !(w->tick&1)) {
            int phase=(int)(w->tick%32);if(w->tick%4==0)blaze_sound(w,BLAZE_SHT);if(phase>=16)phase=31-phase;
            fire_storm_create(w,phase*4-30);*accepted=fire_storm_create(w,30-phase*4);
        }
        return 1;
    case 52:return 1; /* FIRE STORM has no release action in 0x410db0. */
    case 60:case 61:
        if(action==61)p[0x68/4]=0;
        if(!energy(w,1))return 1;
        s=p+0x17f4/4;s[4]=1;
        if(action==60) {
            center(w,s,p+0xb4/4,2,2);
            for(i=0;i<6;++i)memcpy(s+0x34/4+i*4,s+5,16);
            blaze_sound(w,BLAZE_SUU);
        }
        p[0x64/4]=p[0x68/4]=4;*accepted=1;return 1;
    case 62:return 1; /* S-LASER has no release action in 0x410db0. */
    case 112:
        charge_clear(p+0x42d4/4);
        if(!p[0x3e4c/4] && !p[0x3f08/4] && energy(w,12)) {
            p[0x64/4]=8;p[0x68/4]=32;
            blaze_player_move(&w->player,w->level,w->trig,192,w->camera_x);
            blaze_sound(w,BLAZE_BOSU);nova_create(w,p+0x3e3c/4,1);nova_create(w,p+0x3ef8/4,UINT32_MAX);
        }
        return 1; /* Original action returns zero even after creating both. */
    case 122:
        if(energy(w,24)) {
            uint32_t *charge=p+0x42d4/4;s=p+0x1f0/4;
            for(i=0;i<8;++i) {s[0x34/4+i]=charge[0x34/4+i];s[0x54/4+i]=charge[0x74/4+i];}
            thunder_create(w,s);
        }
        charge_clear(p+0x42d4/4);return 1;
    case 132:
        charge_clear(p+0x42d4/4);
        if(blaze_signed(p[0xc8/4])>28 && energy(w,1))for(i=0;i<3;++i) {
            s=p+(0x14e0+i*0x38)/4;if(s[4])continue;
            s[3]=0;s[4]=1;break;
        }
        return 1; /* Original returns zero even when a slot starts. */
    case 140:
        for(i=0;i<16;++i)if(p[(0x347c+i*0x34+0x10)/4])return 1;
        blaze_sound(w,BLAZE_SUN);p[0x64/4]=4;p[0x68/4]=32;
        for(i=0;i<16;++i) {
            s=p+(0x347c+i*0x34)/4;s[3]=s[4]=1;center(w,s,p+5,0,0);s[11]=i*16;
        }
        *accepted=1;return 1;
    case 150:
        s=p+0x4228/4;s[3]+=2;
        if(blaze_signed(s[3])>31)s[3]=32;
        else if(!s[4]) {s[4]=1;*accepted=1;}
        return 1;
    default:snprintf(error,capacity,"Weapon action %u is not reconstructed",action);return 0;
    }
}
void blaze_stock_create(BlazeWorld *w) {
    uint32_t *p=w->player.words,*s=p+0x3ff0/4;unsigned i;
    if(s[4])return;
    s[3]=0;s[4]=1;p[0x64/4]=p[0x68/4]=48;blaze_sound(w,BLAZE_PUU);
    for(i=0;i<64;++i) {
        s[0x38/4+i*2]=blaze_random_next(w->random)*288/32767;
        s[0x3c/4+i*2]=(uint32_t)((int)(blaze_random_next(w->random)*112/32767)-56);
    }
    blaze_player_move(&w->player,w->level,w->trig,192,w->camera_x);
}
static int stock(BlazeWorld *w) {
    uint32_t *p=w->player.words,*s=p+0x3ff0/4;unsigned i,side,n;
    if(!s[4])return 1;
    if(s[3]<8) {
        for(i=0;i<16;++i) {
            BlazeActorDraw *d;int x=(int)(blaze_random_next(w->random)*320/32767)-320;
            int y=(int)(blaze_random_next(w->random)*240/32767);
            int width=(int)(blaze_random_next(w->random)*640/32767);
            if(w->effect_draw_count>=sizeof(w->effect_draws)/sizeof(w->effect_draws[0]))return 0;
            d=&w->effect_draws[w->effect_draw_count++];memset(d,0,sizeof(*d));d->sprite=UINT32_MAX-2;
            d->x=x;d->y=y;d->pivot_x=width;d->pivot_y=1;d->palette=254;
        }
        for(i=0;i<256;++i)if(w->actors[i][0x44/4]&128)w->actors[i][4]=0;
        /* Retire all eight 0x2ddc records as well as enemy projectiles. */
        for(i=0;i<8;++i)p[(0x2ddc+i*0xd4+0x10)/4]=0;
    } else {
        if(s[3]%3==0)blaze_sound(w,BLAZE_LIG);
        if(s[3]&1) {
            resize(w,s,0x168);
            for(side=0;side<2;++side) {
                center(w,s,p+5,0,0);s[9]=side?(uint32_t)-64:64;s[10]=0;
                if(!side)move(s);
                for(n=0;n<(s[3]-8)/2 && n<6;++n) {
                    if(!draw(w,s,0,1,-1))return 0;
                    blaze_weapon_hit(w,s,8,0);blaze_weapon_hit(w,s,16,1);move(s);
                }
            }
        }
    }
    if(++s[3]>64)s[4]=0;return 1;
}
static int force_jet(BlazeWorld *w) {
    uint32_t *p=w->player.words,*s=p+0x1588/4;unsigned side,i,angle;
    if(s[4]) {
        for(side=0;side<2;++side) {
            center(w,s,p+0xb4/4,10,0);angle=(p[11]+(side?192:64))&255;
            s[9]=(uint32_t)(w->trig[angle]/20);s[10]=(uint32_t)(-w->trig[angle+64]/20);
            for(i=0;i<(s[3]<8?(8-s[3])*2:0);++i) {
                if(side?!draw(w,s,s[3]&1,1,-1):!rotate(w,s,s[3]&1,p[11]+64))return 0;
                move(s);
            }
        }
        if(++s[3]>8) {blaze_sound(w,BLAZE_LIG);s[4]=0;}
    }
    s=p+0x15bc/4;
    if(s[4]) {
        center(w,s,p+0xb4/4,-8,6);s[9]=16;s[10]=0;
        for(i=0;i<32;++i) {
            move(s);memcpy(s+0x38/4+i*4,s+5,16);s[0x34/4]=i;
            if(!draw(w,s,(w->tick+32+i)%6,1,-1))return 0;
            if(blaze_weapon_hit(w,s,7,0)) {blaze_weapon_impact(w,w->trig,s+5,0);break;}
        }
        s[4]=0;
    }
    return 1;
}
/* 0x413410: one y-goal per shot, eight position samples, then impact frames. */
static int metal(BlazeWorld *w) {
    unsigned i,j;
    for(i=0;i<24;++i) {
        uint32_t *s=w->player.words+(0x1f28+i*0x78)/4;const BlazeSprite *sp;
        if(!s[4])continue;sp=blaze_level_sprite(w->level,s[0]);
        if(!s[3])s[0x34/4]=blaze_random_next(w->random)*256/32767+sp->visible_height/2-128+s[6];
        s[11]=blaze_turn(w->trig,s[11],w->camera_x+320,blaze_signed(s[0x34/4]),
            blaze_signed(s[5]+sp->width/2),blaze_signed(s[6]+sp->visible_height/2),4);
        if(s[3]==12)for(j=0;j<8;++j) {s[0x38/4+j*2]=s[5];s[0x3c/4+j*2]=s[6];}
        if(s[3]<12) {
            s[11]=blaze_turn(w->trig,s[11],w->camera_x+320,blaze_signed(s[6]+sp->visible_height/2),
                blaze_signed(s[5]+sp->width/2),blaze_signed(s[6]+sp->visible_height/2),2);
            if(!rotate(w,s,0,s[11]))return 0;move(s);
        }
        if(s[3]>11 && s[3]<40) {
            for(j=0;j<8;++j) {
                BlazeActorDraw *d;if(!draw(w,s,0,0,-1))return 0;
                d=&w->effect_draws[w->effect_draw_count-1];d->sprite=0x6f;
                d->x=blaze_signed(s[0x38/4+j*2])-w->camera_x;d->y=blaze_signed(s[0x3c/4+j*2])-w->camera_y;
            }
            memmove(s+0x38/4,s+0x40/4,7*8);s[0x70/4]=s[5];s[0x74/4]=s[6];
            s[9]=(uint32_t)(w->trig[s[11]&255]/10);s[10]=(uint32_t)(-w->trig[(s[11]&255)+64]/10);
            if(!rotate(w,s,1,s[11]))return 0;
            if(blaze_weapon_hit(w,s,10,0))s[3]=40;move(s);
        }
        if(s[3]>39) {
            BlazeActorDraw *d;if(!draw(w,s,0,0,-1))return 0;
            d=&w->effect_draws[w->effect_draw_count-1];d->sprite=0xa1+(s[3]&1);
        }
        if(++s[3]>44)s[4]=0;
    }
    return 1;
}
int blaze_weapons_update(BlazeWorld *w) {
    uint32_t *p=w->player.words,*s;unsigned i;const BlazeWeapon *weapon=&w->loadout->weapons[p[0xb0/4]];
    /* Mount rectangle is computed after scroll/actor pass 0, before effects. */
    p[0xb4/4]=p[5]+(uint32_t)weapon->mount_x[w->loadout->ship]+44;
    p[0xb8/4]=p[6]+(uint32_t)weapon->mount_y[w->loadout->ship]+8;
    p[0xbc/4]=p[0xb4/4]+24;p[0xc0/4]=p[0xb8/4]+8;
    /* 0x416050 presents equipment before actor pass 1 can collect a new
       weapon. Retain the submitted command, just like the other draw streams. */
    memset(&w->mount_draw,0,sizeof(w->mount_draw));
    if(p[0xb0/4]) {
        w->mount_draw.sprite=p[0xb0/4]+0x156;
        w->mount_draw.x=blaze_signed(p[0xb4/4])-w->camera_x;
        w->mount_draw.y=blaze_signed(p[0xb8/4])-w->camera_y;
        w->mount_draw.scale=128;w->mount_draw.palette=-1;
    }
    if(!stock(w))return 0;
    for(i=0;i<10;++i) {
        unsigned angle,j;
        s=p+(0x2ba0+i*0x34)/4;if(!s[4])continue;
        center(w,s,p+0xb4/4,16,0);angle=s[11]&255;
        s[9]=(uint32_t)(w->trig[angle]/28);s[10]=(uint32_t)(-w->trig[angle+64]/28);
        for(j=0;j<s[3];++j)move(s);
        if(!scaled(w,s,0,384-(int)s[3]*10,1))return 0;
        if(blaze_weapon_hit(w,s,8,0))s[4]=0;
        if(++s[3]>16)s[4]=0;
    }
    s=p+0x17f4/4;
    if(s[4]) {
        unsigned row,j;
        center(w,s,p+0xb4/4,2,2);s[9]=16;s[10]=0;
        memmove(s+0x34/4,s+0x44/4,5*16);memcpy(s+0x84/4,s+5,16);
        for(row=0;row<6;++row) {
            unsigned frame=5-row;memcpy(s+5,s+0x34/4+row*4,16);
            for(j=0;j<20;++j) {
                move(s);if(!draw(w,s,frame,1,-1))return 0;
                if((frame==0 || frame==5) && blaze_weapon_hit(w,s,1,0))blaze_weapon_impact(w,w->trig,s+5,1);
            }
        }
        s[4]=0;
    }
    s=p+0x2da8/4;
    if(s[4] && !(w->tick&1)) {
        const BlazeSprite *sp=blaze_level_sprite(w->level,s[0]);
        s[5]=p[5]+48;s[6]=p[6]+16;s[7]=s[5]+sp->width;s[8]=s[6]+sp->visible_height;
        if(!draw(w,s,s[3]%3,1,-1))return 0;
        if(blaze_weapon_hit(w,s,20,0))blaze_weapon_impact(w,w->trig,s+5,1);
        if(++s[3]>3)s[4]=0;
    }
    if(!force_jet(w) || !metal(w))return 0;
    for(i=0;i<32;++i) {
        s=p+(0x4368+i*0x34)/4;if(!s[4])continue;
        s[9]=32;s[10]=s[11];move(s);
        if(!draw(w,s,1,1,-1))return 0;
        if(blaze_weapon_hit(w,s,4,0)) {s[4]=0;blaze_weapon_impact(w,w->trig,s+5,2);}
        if(++s[3]>16)s[4]=0;
    }
    return 1;
}
/* 0x413c80: build once, then animate owned rectangles/angles for 12 updates.
   Count caps intentionally exclude the final stored slot, as in the original. */
int blaze_thunder_update(BlazeWorld *w) {
    uint32_t *s=w->player.words+0x1f0/4;unsigned i,j,n;
    const BlazeSprite *sp=blaze_level_sprite(w->level,s[0]);
    if(!s[4])return 1;
    if(!sp)return 0;
    if(!s[3]) {
        s[5]=(uint32_t)w->camera_x+160+blaze_random_next(w->random)*160/32767;
        s[6]=(uint32_t)w->camera_y-64;s[7]=s[5]+sp->width;s[8]=s[6]+sp->visible_height;
        s[0x874/4]=s[0x878/4]=0;
        for(i=0;i<9;++i) {
            uint32_t *a=NULL;int tx=w->camera_x+240,ty=w->camera_y+368;
            if(i<8) {
                const BlazeSprite *as;
                if(!s[0x34/4+i])continue;
                a=target(w,s[0x54/4+i]);if(!a)return 0;
                as=target_sprite(w,a[0]);if(!as)return 0;
                tx=blaze_signed(a[5]+as->width/2);ty=blaze_signed(a[6]+as->visible_height/2);
            }
            for(n=0;n<(a?64u:96u);++n) {
                unsigned angle,index;
                for(j=0;j<8;++j)s[11]=blaze_turn(w->trig,s[11],tx,ty,
                    blaze_signed(s[5]+sp->width/2),blaze_signed(s[6]+sp->visible_height/2),4);
                s[11]+=blaze_random_next(w->random)*16/32767-8;angle=s[11]&255;
                s[9]=(uint32_t)(w->trig[angle]/10);s[10]=(uint32_t)(-w->trig[angle+64]/10);move(s);
                index=s[0x874/4];memcpy(s+0xf4/4+index*4,s+5,16);s[0x6f4/4+index]=s[11];
                if(++s[0x874/4]>95)s[0x874/4]=95;
                if(a && blaze_records_overlap(s,a)) {
                    index=s[0x878/4];memcpy(s+0x74/4+index*4,s+5,16);
                    if(++s[0x878/4]>7)s[0x878/4]=7;
                    a[0x50/4]-=70;a[0x64/4]=70;break;
                }
                if(!a && blaze_signed(s[6])>w->camera_y+336)break;
            }
        }
    }
    if(s[0x874/4]>95 || s[0x878/4]>7)return 0;
    for(i=0;i<s[0x874/4];++i) {
        unsigned frame=blaze_random_next(w->random)*4/32767+4+(s[3]/3)*4;
        memcpy(s+5,s+0xf4/4+i*4,16);
        if(!rotate(w,s,frame,s[0x6f4/4+i]))return 0;
    }
    for(i=0;i<s[0x878/4];++i) {memcpy(s+5,s+0x74/4+i*4,16);if(!draw(w,s,s[3]/3,0,-1))return 0;}
    if(++s[3]>11)s[4]=0;return 1;
}
/* 0x4146d0: player owns three reusable pulses. The unused final word is
   preserved; 0x4146b0 only sets age/active. Its RET 8 discards two unused
   stack arguments, including the dispatcher's apparent direction value. */
/* 0x4139e0: five original player-owned records. No creator is reachable in
   the recovered action dispatcher; retain the actual update/lifetime rather
   than inventing a new weapon action for this dormant pool. */
int blaze_scatter_bursts_update(BlazeWorld *w) {
    unsigned i,j;uint32_t *p=w->player.words;
    for(i=0;i<5;++i) {
        uint32_t *s=p+(0xec+i*0x34)/4,rect[4];int age=blaze_signed(s[3]);
        if(!s[4])continue;
        if(age<32) {
            unsigned angle=s[11]&255;
            s[9]=(uint32_t)(w->trig[angle]/(age+8));s[10]=(uint32_t)(-w->trig[angle+64]/(age+8));move(s);
            if(!rotate(w,s,s[3]&1,s[11]))return 0;
        } else {
            blaze_random_next(w->random);memcpy(rect,s+5,sizeof(rect));resize(w,s,0x7d);s[12]=(uint32_t)-2;
            for(j=0;j<8;++j) {
                int y=(int)(blaze_random_next(w->random)*192/32767)-96;
                int x=(int)(blaze_random_next(w->random)*192/32767)-96;
                unsigned frame;
                center(w,s,rect,x,y);frame=blaze_random_next(w->random)*2/32767;
                if(!draw(w,s,frame,1,-1))return 0;
                blaze_weapon_hit(w,s,4,0);
            }
            memcpy(s+5,rect,sizeof(rect));
        }
        if(++s[3]>48)s[4]=0;
    }
    return 1;
}
int blaze_gbraster_update(BlazeWorld *w) {
    uint32_t *p=w->player.words;unsigned i,j,k;
    for(i=0;i<3;++i) {
        uint32_t *s=p+(0x14e0+i*0x38)/4;
        if(!s[4])continue;
        if(s[3]==1)blaze_sound(w,BLAZE_GB);
        if((s[3]&1) && s[3]<25) {
            center(w,s,p+5,112,0);resize(w,s,0xe1);
            if(!draw(w,s,0,1,-1))return 0;
            blaze_weapon_hit(w,s,22,0);
            resize(w,s,0xc9+blaze_random_next(w->random)*2/32767*4);
            for(j=0;j<4;++j) {
                s[11]=blaze_random_next(w->random)*256/32767;
                center(w,s,p+5,112,0);
                for(k=0;k<16;++k) {
                    unsigned frame=blaze_random_next(w->random)*4/32767,angle=s[11]&255;
                    if(!rotate(w,s,frame,s[11]))return 0;
                    s[9]=(uint32_t)(w->trig[angle]/12);s[10]=(uint32_t)(-w->trig[angle+64]/12);move(s);
                    s[11]+=blaze_random_next(w->random)*64/32767-32;
                }
            }
        }
        if(s[3]>24) {
            center(w,s,p+5,112,0);resize(w,s,0xe1);
            if(!scaled(w,s,0,(32-(int)s[3])*32,0))return 0;
        }
        if(++s[3]>32)s[4]=0;
    }
    return 1;
}
/* 0x411630: shared charge work is consumed and cleared each frame. */
int blaze_charge_update(BlazeWorld *w) {
    uint32_t *p=w->player.words,*s=p+0x42d4/4;unsigned frame;
    if(!s[4])return 1;
    if(s[11]==0) {
        if(blaze_signed(p[0xc8/4])>32)p[0xc8/4]=32;
        resize(w,s,0xd9);center(w,s,p+0xb4/4,16,0);frame=w->tick&3;
    } else if(s[11]==2) {
        if(blaze_signed(p[0xc8/4])>64)p[0xc8/4]=64;
        resize(w,s,0xb8);center(w,s,p+0xb4/4,0,2);
        frame=(w->tick&1)+(blaze_signed(p[0xc8/4])<11?0:2);
    } else if(s[11]==3) {
        if(blaze_signed(p[0xc8/4])>64)p[0xc8/4]=64;
        resize(w,s,0x5d);center(w,s,p+0xb4/4,0,4);
        frame=(w->tick/4)%2+(blaze_signed(p[0xc8/4])<29?0:2);
    } else if(s[11]==1) {
        unsigned i,j;
        resize(w,s,0xb2);center(w,s,p+0xb4/4,118,8);
        if(!draw(w,s,0,0,-1))return 0;
        for(i=0;i<256;++i) {
            uint32_t *a=w->actors[i];unsigned slot,count=0;
            if((a[0x44/4]&(128|1024)) || !blaze_records_overlap(s,a))continue;
            for(slot=0;slot<8 && s[0x34/4+slot];++slot) { }
            if(slot==8)continue;
            for(j=0;j<8;++j)count+=s[0x74/4+j]==i+1;
            if(!count || (blaze_signed(s[3])>0 && (int)(8-count)<blaze_signed(s[3]))) {
                s[0x74/4+slot]=i+1;s[0x34/4+slot]=1;s[0x54/4+slot]=0;blaze_sound(w,BLAZE_PI);s[3]=0;
            }
        }
        for(i=0;i<8;++i)if(s[0x34/4+i]) {
            BlazeActorDraw *d;const BlazeSprite *as;unsigned age;
            uint32_t *a=target(w,s[0x74/4+i]);if(!a)return 0;
            as=target_sprite(w,a[0]);if(!as)return 0;
            if(blaze_signed(a[5])>=w->camera_x+448 || blaze_signed(a[7])<=w->camera_x-128 ||
               blaze_signed(a[6])>=w->camera_y+368 || blaze_signed(a[8])<=w->camera_y-128 ||
               !a[4] || blaze_signed(a[0x58/4])>99)s[0x34/4+i]=0;
            if(++s[0x54/4+i]>4)s[0x54/4+i]=4;age=s[0x54/4+i];
            if(!draw(w,s,0,0,-1))return 0;
            d=&w->effect_draws[w->effect_draw_count-1];d->sprite=age+0xb3;d->scale=1024-(int)age*192;
            d->x=blaze_signed(a[5]+as->width/2)-w->camera_x+((int)age-6)*8;
            d->y=blaze_signed(a[6]+as->visible_height/2)-w->camera_y+((int)age-6)*8;
        }
        ++s[3];s[4]=0;return 1;
    } else return 0;
    if(!draw(w,s,frame,0,-1))return 0;
    ++s[3];s[4]=0;return 1;
}
int blaze_secondary_update(BlazeWorld *w) {
    uint32_t *p=w->player.words,*s;unsigned i,j,angle,ship=w->loadout->ship;int dx,dy;
    for(i=0;i<16;++i) {
        s=p+(0x347c+i*0x34)/4;if(!s[4])continue;
        if(!draw(w,s,0,1,-1))return 0;
        s[11]+=ship==0?16:ship==1?32:8;angle=s[11]&255;
        s[9]=(uint32_t)(w->trig[angle]/8);s[10]=(uint32_t)(-w->trig[angle+64]/8);move(s);
        if(blaze_weapon_hit(w,s,10,1))blaze_weapon_impact(w,w->trig,s+5,2);
        if(blaze_weapon_hit(w,s,1,0))blaze_weapon_impact(w,w->trig,s+5,2);
        if(++s[3]>31)s[4]=0;
    }
    for(i=0;i<2;++i) {
        uint32_t head[4];unsigned segment;int wave;
        s=p+(0x3e3c+i*0xbc)/4;if(!s[4])continue;
        wave=(int)(s[3]&15)*8;if(wave>=63)wave=125-wave;if(wave<0)wave=0;
        angle=((uint32_t)(wave-32)*s[0x34/4]+256+s[11])&255;
        s[9]=(uint32_t)(w->trig[angle]/12);s[10]=(uint32_t)(-w->trig[angle+64]/12);move(s);
        memcpy(head,s+5,16);memcpy(s+0xa8/4,head,16);
        memmove(s+0x38/4,s+0x48/4,7*16);
        for(segment=0;segment<7;++segment) {
            resize(w,s,0x88);s[12]=s[0xb8/4]-8;memcpy(s+5,s+0x38/4+segment*4,16);
            if(!scaled(w,s,0,16+(int)segment*16+(int)s[0xb8/4]*20,1))return 0;
            blaze_weapon_hit(w,s,1,0);
        }
        memcpy(s+5,head,16);if(++s[3]>31)s[4]=0;
    }
    s=p+0x4228/4;if(!s[4])return 1;
    memmove(s+0x34/4,s+0x44/4,5*16);memmove(s+0x94/4,s+0x98/4,5*4);
    memcpy(s+0x84/4,p+5,16);s[0xa8/4]=p[0x74/4];
    if(blaze_signed(p[0xd0/4])<3)for(i=0;i<6;++i) {memcpy(s+0x34/4+i*4,p+5,16);s[0x94/4+i]=p[0x74/4];}
    for(i=0;i<6;++i) {
        resize(w,s,p[0]+s[0x94/4+i]);center(w,s,s+0x34/4+i*4,0,0);
        if(!draw(w,s,0,0,3+5-(int)i))return 0;
    }
    if(p[0xd0/4]==1)blaze_sound(w,BLAZE_BACK);
    if(ship==0) {
        if(blaze_signed(p[0xd0/4])<3) {
            for(i=0;i<8;++i) {
                unsigned direction=blaze_random_next(w->random)*128/32767+128;
                uint32_t *spark=p+(0x37bc+i*0x34)/4;
                if(spark[4])continue;
                center(w,spark,s+5,0,0);spark[11]=direction;angle=direction&255;
                spark[9]=(uint32_t)(w->trig[angle]/8);spark[10]=(uint32_t)(-w->trig[angle+64]/8);
                spark[3]=spark[4]=1;resize(w,spark,0x61);
            }
        } else if(w->tick&1) {resize(w,s,0x87);center(w,s,p+5,-32,2);if(!rotate(w,s,0,p[11]))return 0;}
        blaze_weapon_hit(w,s,7,0);
        if(p[0xe4/4]!=UINT32_MAX)blaze_player_move(&w->player,w->level,w->trig,p[0xe4/4],w->camera_x);
        p[0xe4/4]=UINT32_MAX;
    } else if(ship==1) {
        resize(w,s,0x13d);dy=(int)(blaze_random_next(w->random)*8/32767)-4;
        dx=(int)(blaze_random_next(w->random)*8/32767)-4;center(w,s,p+5,dx,dy);
        if(w->tick&1) {j=blaze_random_next(w->random)*256/32767;if(!rotate(w,s,0,j))return 0;}
        blaze_weapon_hit(w,s,5,0);blaze_weapon_hit(w,s,58,1);
    }
    return 1;
}

/* FRASH KNIFE, 0x4141f0: only age zero strikes; later ages retain its record. */
int blaze_knife_update(BlazeWorld *w) {
    uint32_t *s=w->player.words+0x2a68/4;
    if(!s[4])return 1;
    resize(w,s,0x142);center(w,s,w->player.words+0xb4/4,16,16);
    if(!s[3]) {
        if(!draw(w,s,0,1,-1))return 0;
        blaze_sound(w,BLAZE_KIL);
        if(blaze_weapon_hit(w,s,98,0))blaze_weapon_impact(w,w->trig,s+5,0);
        if(blaze_weapon_hit(w,s,64,1))blaze_weapon_impact(w,w->trig,s+5,0);
    }
    if(++s[3]>6)s[4]=0;return 1;
}
/* 0x419880 directly converts the angle to the original 256-turn convention.
   Wrapped integer differences precede atan; the horizontal case uses 0.001. */
unsigned blaze_aim(int tx,int ty,int sx,int sy) {
    double dy=blaze_signed((uint32_t)ty-(uint32_t)sy);
    double dx=blaze_signed((uint32_t)sx-(uint32_t)tx);
    int negative=dy<0,angle;
    if(dy==0)dy=0.001;
    angle=-(int)(atan(dx/dy)*-40.74366543152252)-128;
    if(negative)angle+=128;return (unsigned)(angle+512)&255;
}
static void clamp_drone(BlazeWorld *w,uint32_t *s) {
    const BlazeSprite *sp=blaze_level_sprite(w->level,s[0]);
    if(blaze_signed(s[5])<w->camera_x) {s[5]=(uint32_t)w->camera_x;s[7]=s[5]+sp->width;}
    if(blaze_signed(s[7])>=w->camera_x+320) {s[7]=(uint32_t)w->camera_x+320;s[5]=s[7]-sp->width;}
    if(blaze_signed(s[6])<0) {s[6]=0;s[8]=sp->visible_height;}
    if(blaze_signed(s[8])>=320) {s[8]=320;s[6]=320-sp->visible_height;}
}
/* 0x412650: drones own no target. A checked slot handle follows slot reuse.
   The temporary beam is frame-local scratch, replacing the old root scratch. */
int blaze_marker_drones_update(BlazeWorld *w) {
    uint32_t *p=w->player.words;unsigned i,j;
    for(i=0;i<3;++i) {
        uint32_t *s=p+(0xa6c+i*0x3c)/4;
        if(!s[4])continue;
        s[5]+=(uint32_t)w->scroll_speed;s[7]+=(uint32_t)w->scroll_speed;
        if(s[3]<6) {
            unsigned angle=s[11]&255;s[9]=(uint32_t)(w->trig[angle]/12);s[10]=(uint32_t)(-w->trig[angle+64]/12);
            move(s);clamp_drone(w,s);
        }
        if(!p[(0x3fb4+0x10)/4])s[4]=0;
        else {
            uint32_t *a=target(w,s[0x38/4]);const BlazeSprite *as,*sp=blaze_level_sprite(w->level,s[0]);
            if(!a)return 0;as=target_sprite(w,a[0]);if(!as)return 0;
            s[0x34/4]=blaze_aim(blaze_signed(a[5]+as->width/2),blaze_signed(a[6]+as->visible_height/2),
                blaze_signed(s[5]+sp->width/2),blaze_signed(s[6]+sp->visible_height/2));
            if(s[3]==8)blaze_sound(w,BLAZE_SUU);
            if(s[3]>8 || s[3]==6) {
                uint32_t beam[13]={0x51};unsigned angle=s[0x34/4]&255;
                beam[12]=(uint32_t)-2;center(w,beam,s+5,0,0);
                for(j=0;j<32;++j) {
                    beam[9]=(uint32_t)(w->trig[angle]/20);beam[10]=(uint32_t)(-w->trig[angle+64]/20);move(beam);
                    if(blaze_random_next(w->random)*4/32767) {
                        if(!rotate(w,beam,s[3]&1,s[0x34/4]))return 0;
                        if(blaze_weapon_hit(w,beam,3,0))break;
                    }
                }
            }
        }
        resize(w,s,0xad);if(!rotate(w,s,s[3]&3,s[0x34/4]))return 0;
        if(++s[3]>48)s[4]=0;
    }
    return 1;
}
/* 0x415240 runs after charge/foreground. Acquisition starts drones for the
   following frame, and retirement still submits one final target marker. */
int blaze_marker_update(BlazeWorld *w) {
    uint32_t *p=w->player.words,*s=p+0x3fb4/4;unsigned i;
    if(!s[4])return 1;
    if(!s[0x38/4]) {
        move(s);if(!draw(w,s,s[3]&1,0,-1))return 0;
        for(i=0;i<256;++i)if(!(w->actors[i][0x44/4]&(128|1024)) && blaze_records_overlap(s,w->actors[i])) {
            unsigned j;static const unsigned directions[]={0,128,192};
            s[0x38/4]=1;s[0x34/4]=i+1;blaze_sound(w,BLAZE_PI);s[3]=0;
            for(j=0;j<3;++j) {
                uint32_t *d=p+(0xa6c+j*0x3c)/4;if(d[4])continue;
                d[11]=directions[j];d[4]=1;d[0x34/4]=(directions[j]+128)%128;d[0x38/4]=i+1;d[3]=0;
                resize(w,d,0xad);center(w,d,p+5,0,0);
            }
            break;
        }
        if(s[3]++>32)s[4]=0;
    } else {
        BlazeActorDraw *d;uint32_t *a=target(w,s[0x34/4]);const BlazeSprite *as;
        if(!a)return 0;as=target_sprite(w,a[0]);if(!as)return 0;
        if(blaze_signed(a[5])>=w->camera_x+384 || blaze_signed(a[7])<=w->camera_x-64 ||
           blaze_signed(a[6])>=w->camera_y+304 || blaze_signed(a[8])<=w->camera_y-64 ||
           !a[4] || blaze_signed(a[0x58/4])>99)s[4]=0;
        if(!draw(w,s,0,0,-1))return 0;d=&w->effect_draws[w->effect_draw_count-1];d->sprite=0xb7;
        d->x=blaze_signed(a[5]+as->width/2)-w->camera_x-16;d->y=blaze_signed(a[6]+as->visible_height/2)-w->camera_y-16;
        if(s[3]++>70)s[4]=0;
    }
    return 1;
}

/* 0x411d80/0x411e50: enemy homing trails live in the player's existing eight
   0xd4 records. Stock/reset/unload already retire this same authoritative pool. */
int blaze_enemy_trail_create(BlazeWorld *w,const uint32_t rect[4],unsigned angle) {
    unsigned i,j;uint32_t *s;
    for(i=0;i<8;++i) {
        s=w->player.words+(0x2ddc+i*0xd4)/4;if(s[4])continue;
        s[3]=0;s[4]=1;center(w,s,rect,0,0);
        for(j=0;j<8;++j)memcpy(s+0x54/4+j*4,s+5,16);
        s[11]=angle;s[9]=s[10]=0;return 1;
    }
    return 0;
}
int blaze_enemy_trails_update(BlazeWorld *w) {
    unsigned i,j;const uint32_t *p=w->player.words;
    const BlazeSprite *ps=blaze_level_sprite(w->level,p[0]);
    for(i=0;i<8;++i) {
        uint32_t *s=w->player.words+(0x2ddc+i*0xd4)/4;
        const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);
        if(!s[4])continue;
        if(s[3]%32>8) {
            for(j=0;j<2;++j)s[11]=blaze_turn(w->trig,s[11],
                blaze_signed(p[5]+ps->width/2),blaze_signed(p[6]+ps->visible_height/2),
                blaze_signed(s[5]+a->width/2),blaze_signed(s[6]+a->visible_height/2),3);
            s[9]=(uint32_t)(w->trig[s[11]&255]/20);s[10]=(uint32_t)(-w->trig[(s[11]&255)+64]/20);
        }
        move(s);memcpy(s+0xc4/4,s+5,16);s[0x50/4]=s[11];
        for(j=0;j<7;++j) {memcpy(s+0x54/4+j*4,s+0x64/4+j*4,16);s[0x34/4+j]=s[0x38/4+j];}
        for(j=0;j<8;++j) {
            memcpy(s+5,s+0x54/4+j*4,16);
            if(!rotate(w,s,s[3]&1,s[0x34/4+j]))return 0;
            if(blaze_player_contact(w,s,28)) {s[4]=0;break;}
        }
        memcpy(s+5,s+0xc4/4,16);if(++s[3]>48)s[4]=0;
    }
    return 1;
}
