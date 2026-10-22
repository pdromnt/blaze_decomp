#include "blaze.h"
#include <stdio.h>
#include <string.h>

void blaze_sound(BlazeWorld *w,unsigned sound) {
    if(w->media && w->media->sound)w->media->sound(w->media->context,sound);
}
static int begin_world(BlazeWorld *w,BlazeLevel *level,const BlazeArchive *font,const BlazeLoadout *loadout,BlazeRandom *random,const int32_t trig[320],int restart) {
    size_t i;BlazePlayer carry;
    const BlazeMedia *media=restart?w->media:NULL;
    if(restart)carry=w->player;
    blaze_world_end(w);
    if(restart)w->player=carry;
    w->media=media;w->level=level;w->font=font;w->loadout=loadout;w->scroll_speed=2;w->random=random;w->trig=trig;
    blaze_color_table(w->colors[0],level->tiles.bytes+12,0xffffff,70);
    blaze_color_table(w->colors[1],level->tiles.bytes+12,0xffffff,30);
    blaze_color_table(w->colors[2],level->tiles.bytes+12,0x101010,70);
    for(i=0;i<6;++i)blaze_color_table(w->backdrop_colors[i],font->bytes+12,0x20dc,(unsigned)i*15);
    /* 0x407a60 configures max health before 0x416670 resets the player. */
    w->player.words[0x2ae4/4]=level->difficulty==0?256:level->difficulty==1?192:128;
    if(!(restart?blaze_player_restart(&w->player,level,font,loadout):blaze_player_reset(&w->player,level,font,loadout))) {blaze_world_end(w);return 0;}
    for(i=0;i<256;++i) {
        w->actors[i][0]=4;w->actors[i][7]=font->sprites[4].width;
        w->actors[i][8]=font->sprites[4].visible_height;w->actors[i][12]=(uint32_t)-3;
    }
    w->active=1;return 1;
}
int blaze_world_begin(BlazeWorld *w,BlazeLevel *level,const BlazeArchive *font,const BlazeLoadout *loadout,BlazeRandom *random,const int32_t trig[320]) {
    return begin_world(w,level,font,loadout,random,trig,0);
}
int blaze_world_restart(BlazeWorld *w) {
    if(!w->active || !w->level || !w->font || !w->loadout || !w->random || !w->trig)return 0;
    return begin_world(w,w->level,w->font,w->loadout,w->random,w->trig,1);
}
void blaze_world_end(BlazeWorld *w) { memset(w,0,sizeof(*w)); }
static int boundary(char *error,size_t capacity,const char *reason) {
    snprintf(error,capacity,"%s",reason);return 0;
}
/* 0x410920: shared tap/hold/release, X edge/double tap and stock ordering. */
int blaze_player_step(BlazeWorld *w,const int32_t trig[320],unsigned input,char *error,size_t capacity) {
    uint32_t *s=w->player.words;unsigned direction=0;int moving=1,accepted;
    if(!w->active)return boundary(error,capacity,"Player/world is not constructed");
    /* Type 2 falls before input, including frames with no movement request. */
    if(w->loadout->ship==2) {
        const BlazeSprite *sprite=blaze_level_sprite(w->level,s[0]);
        s[9]=0;s[10]+=1;s[6]+=s[10];s[8]+=s[10];
        if(blaze_signed(s[5])<w->camera_x) {s[5]=(uint32_t)w->camera_x;s[7]=s[5]+sprite->width;}
        if(blaze_signed(s[7])>=w->camera_x+320) {s[7]=(uint32_t)(w->camera_x+320);s[5]=s[7]-sprite->width;}
        if(blaze_signed(s[6])<0) {s[6]=0;s[8]=sprite->visible_height;}
        if(blaze_signed(s[8])>=320) {s[8]=320;s[6]=320-sprite->visible_height;}
    } else s[10]=0;
    if(blaze_signed(s[0x4234/4])>0) --s[0x4234/4];
    s[0x4238/4]=0;
    s[0x78/4]+=6;if(blaze_signed(s[0x78/4])>64)s[0x78/4]=64;
    s[0x7c/4]=0;if(blaze_signed(s[0x68/4])>0)--s[0x68/4];
    s[0xa4/4]=input;s[0x80/4]=0;
    if(blaze_signed(s[0xd0/4])<2 && blaze_signed(s[0xcc/4])<2 && blaze_signed(s[0x5c/4])>0 &&
       !s[0x4000/4] && (input&BLAZE_CONFIRM) && (input&BLAZE_CANCEL) &&
       (s[0x60/4]&(BLAZE_CONFIRM|BLAZE_CANCEL))) {
        --s[0x5c/4];input&=~(BLAZE_CONFIRM|BLAZE_CANCEL);s[0xa4/4]=input;
        blaze_stock_create(w);
    } else {
        s[0x60/4]=input;
        if((input&BLAZE_DOWN)&&(input&BLAZE_LEFT)) direction=160;
        else if((input&BLAZE_UP)&&(input&BLAZE_LEFT)) direction=224;
        else if((input&BLAZE_DOWN)&&(input&BLAZE_RIGHT)) direction=96;
        else if((input&BLAZE_UP)&&(input&BLAZE_RIGHT)) direction=32;
        else if(input&BLAZE_UP) direction=0;
        else if(input&BLAZE_DOWN) direction=128;
        else if(input&BLAZE_LEFT) direction=192;
        else if(input&BLAZE_RIGHT) direction=64;
        else moving=0;
        if(moving) {s[0x34/4]=direction;blaze_player_move(&w->player,w->level,trig,direction,w->camera_x);}
    }
    if(input&BLAZE_CANCEL) {
        if(!blaze_weapon_action(w,150,&accepted,error,capacity))return 0;
        if(!s[0xd0/4]++ && ++s[0xd4/4]>1) {
            if(w->loadout->mode==3) {
                unsigned slot=s[0x3c/4];s[0x4c/4+slot]=s[0x2ad8/4];
                slot=(slot+1)%3;s[0x3c/4]=slot;s[0xb0/4]=s[0x40/4+slot];
                s[0x2adc/4]=0;s[0x2ad8/4]=s[0x4c/4+slot];
                s[0xe8/4]=(uint32_t)w->loadout->weapons[s[0xb0/4]].action;
            }
            if(!blaze_weapon_action(w,140,&accepted,error,capacity))return 0;
            s[0xd4/4]=0;
        }
        if(s[0xd8/4]++>6)s[0xd4/4]=0;
        s[0xdc/4]=s[0xc4/4]=0;s[0xa4/4]&=~BLAZE_CANCEL;
    } else {
        s[0xd0/4]=0;
        if(blaze_signed(s[0xdc/4])>6)s[0xd4/4]=0;
        ++s[0xdc/4];s[0xd8/4]=0;
    }
    if(input&BLAZE_CONFIRM) {
        if(!s[0xcc/4]++ && !blaze_weapon_action(w,10,&accepted,error,capacity))return 0;
        if(s[0xcc/4]==8) {
            if(!blaze_weapon_action(w,s[0xe8/4],&accepted,error,capacity))return 0;
            if(!accepted)s[0xc8/4]=s[0xcc/4]=4;
        }
        if(blaze_signed(s[0xcc/4])>11) {
            if(!blaze_weapon_action(w,s[0xe8/4]+1,&accepted,error,capacity))return 0;
            s[0xcc/4]=32;
        }
        s[0xc4/4]=0;++s[0xc8/4];
    } else {
        if(blaze_signed(s[0xc8/4])>6 && !blaze_weapon_action(w,s[0xe8/4]+2,&accepted,error,capacity))return 0;
        s[0xc8/4]=s[0xcc/4]=0;++s[0xc4/4];blaze_player_regenerate(w);
    }
    if(w->loadout->mode==1 || w->loadout->mode==4) {
        if(!(w->tick&3) || s[0x2adc/4]) {
            s[0x2ad8/4]+=1;
            if(blaze_signed(s[0x2ad8/4])>w->loadout->weapons[s[0xb0/4]].power) {
                s[0x2ad8/4]=(uint32_t)w->loadout->weapons[s[0xb0/4]].power;s[0x2adc/4]=0;
            }
        }
    }
    return 1;
}
/* 0x40533a..0x40564f. Run after camera clipping and before actor registration,
   preserving respawn rolls before any enemy/weapon RNG consumption. */
void blaze_world_backdrop_step(BlazeWorld *w) {
    unsigned j,stage=w->level->stage;
    w->backdrop_phase=w->encounter_phase;
    if(w->encounter_phase>0 && w->encounter_phase<18)++w->encounter_phase;
    if(w->encounter_phase!=18)return;
    w->backdrop_phase=18;
    if(stage==0 || stage==2) {
        static const int centers[4][2]={{40,0},{280,0},{40,240},{280,240}};
        for(j=0;j<4;++j) {
            uint32_t *wave=w->random->waves[j];
            wave[0]+=wave[2];wave[1]+=wave[3];
            w->backdrop_points[j*2]=centers[j][0]+w->trig[(wave[0]&255)+64]/3;
            w->backdrop_points[j*2+1]=centers[j][1]+w->trig[(wave[1]&255)+64]/3+(32-w->camera_y)*2;
        }
    } else if(stage<5) {
        unsigned count=stage==3?40:80;
        for(j=0;j<count;++j) {
            int32_t *p=w->random->streaks[j];p[0]-=48;
            if(p[0]<-128) {
                p[0]=320+(int32_t)(blaze_random_next(w->random)*32/32767);
                p[1]=(int32_t)(blaze_random_next(w->random)*480/32767);
            }
            w->backdrop_streaks[j][0]=p[0];w->backdrop_streaks[j][1]=p[1]-w->camera_y*2;
        }
    }
}
void blaze_world_backdrop_draw(const BlazeWorld *w,BlazeFrame *frame) {
    unsigned stage=w->level->stage,j;
    if(stage==0 || stage==2)blaze_draw_quad(frame,&w->level->tiles,(stage==0?0x2b6:0x2cd)-0x170,w->backdrop_points);
    else if(stage<5)for(j=0;j<(stage==3?40u:80u);++j)
        blaze_draw(frame,&w->level->objects,(stage==3?0x145:0x144)-0x40,
            w->backdrop_streaks[j][0],w->backdrop_streaks[j][1],NULL);
}
/* Recovered world prefix of 0x410920/0x4050a0. Stop at the first
   unsupported behavior, keeping pending spawn data intact. No route is
   replaced with a fabricated success or an original-code callback. */
int blaze_world_step(BlazeWorld *w,const int32_t trig[320],unsigned input,char *error,size_t capacity) {
    uint32_t *s=w->player.words; size_t i;
    if(!w->active) return boundary(error,capacity,"World is not constructed");
    w->shot_draw_count=w->primary_draw_count=0;w->actor_draw_count=w->actor_pass0_count=0;
    w->effect_draw_count=w->effect_primary_count=w->effect_weapon_count=w->effect_player_count=w->effect_thunder_count=w->effect_gbraster_count=0;
    for(i=0;i<blaze_player_pool_count;++i) {
        const BlazePool *pool=&blaze_player_pools[i];size_t j;
        if(pool->offset==0xec || pool->offset==0xb20 || pool->offset==0x37bc || pool->offset==0x1888 ||
           pool->offset==0x1d88 || pool->offset==0x2ae8 || pool->offset==0x4368 ||
           pool->offset==0x2ddc || pool->offset==0x347c || pool->offset==0x4228 || pool->offset==0x3ff0 ||
           pool->offset==0x1588 || pool->offset==0x15bc || pool->offset==0x42d4 ||
           pool->offset==0x2da8 || pool->offset==0x2ba0 || pool->offset==0x3e3c ||
           pool->offset==0x17f4 || pool->offset==0x1f0 || pool->offset==0x14e0 || pool->offset==0x1f28 ||
           pool->offset==0x2a68 || pool->offset==0x3fb4 || pool->offset==0xa6c)continue;
        for(j=0;j<pool->count;++j)if(s[(pool->offset+j*pool->stride+0x10)/4])
            return boundary(error,capacity,"Active weapon/effect update is not reconstructed");
    }
    ++w->tick;
    if(!(w->tick&7)) {
        int target=w->loadout->ship==0?6:w->loadout->ship==1?7:3;
        w->scroll_speed+=(w->scroll_speed<target)-(w->scroll_speed>target);
    }
    if(!blaze_player_step(w,trig,input,error,capacity))return 0;
    if(!w->dying) {
    w->camera_y=(blaze_signed(s[6])-240)/2+120;
    if(w->camera_x>=0x5000-360)w->scroll_speed=0;
    w->camera_x+=w->scroll_speed;s[5]+=(uint32_t)w->scroll_speed;s[7]+=(uint32_t)w->scroll_speed;
    for(i=0;i<256;++i)if(!(w->actors[i][0x44/4]&8)) {
        w->actors[i][5]+=(uint32_t)w->scroll_speed;w->actors[i][7]+=(uint32_t)w->scroll_speed;
    }
    if(w->camera_y<0)w->camera_y=0;
    if(w->camera_y>=80)w->camera_y=79;
    }
    /* Backgrounds continue during player death, when camera motion is frozen.
       A warning created by actors below affects the following frame. */
    blaze_world_backdrop_step(w);
    for(i=0;i<128;++i) {
        BlazeSpawn *spawn=&w->level->spawns[i];
        if(blaze_signed(spawn->x)<w->camera_x+320 && spawn->flags) {
            if(!blaze_actor_register(w,spawn)) {
                snprintf(error,capacity,"Actor type %u at spawn %zu requires registration/update chain 0x4076b0 -> 0x407fc0 -> 0x409fb0",spawn->parameter,i);
                return 0;
            }
            spawn->flags=0;
        }
    }
    for(i=0;i<256;++i)if(!blaze_actor_update(w,w->actors[i],0,error,capacity))return 0;
    w->actor_pass0_count=w->actor_draw_count;
    if(!blaze_weapons_update(w))return boundary(error,capacity,"Weapon draw capacity exceeded");
    w->effect_primary_count=w->effect_draw_count;
    for(i=0;i<48;++i)if(!blaze_primary_update(w,trig,s+(0xb20+i*0x34)/4))
        return boundary(error,capacity,"Primary shot draw capacity exceeded");
    w->primary_draw_count=w->shot_draw_count;
    if(!blaze_marker_drones_update(w))return boundary(error,capacity,"MARKER RAY drone target/draw capacity exceeded");
    if(!blaze_secondary_update(w))return boundary(error,capacity,"Secondary draw capacity exceeded");
    w->effect_weapon_count=w->effect_draw_count;
    if(!blaze_player_present_effects(w))return boundary(error,capacity,"Player effect draw capacity exceeded");
    if(!blaze_knife_update(w))return boundary(error,capacity,"FRASH KNIFE draw capacity exceeded");
    w->effect_player_count=w->effect_draw_count;
    if(!blaze_thunder_update(w))return boundary(error,capacity,"THUNDERBOLT target/draw capacity is not supported");
    w->effect_thunder_count=w->effect_draw_count;
    if(!blaze_sparks_update(w))return boundary(error,capacity,"Spark draw capacity exceeded");
    if(!blaze_gbraster_update(w))return boundary(error,capacity,"G-BRASTER draw capacity exceeded");
    if(!blaze_scatter_bursts_update(w))return boundary(error,capacity,"Scatter burst draw capacity exceeded");
    if(!blaze_enemy_trails_update(w))return boundary(error,capacity,"Enemy trail draw capacity exceeded");
    w->effect_gbraster_count=w->effect_draw_count;
    for(i=0;i<256;++i) {
        w->actors[i][0x5c/4]=UINT32_MAX;
        if(!blaze_actor_update(w,w->actors[i],1,error,capacity))return 0;
    }
    if(blaze_signed(s[0x2ad0/4])<1 && !w->dying && !w->requested_screen) {if(w->media && w->media->stop)w->media->stop(w->media->context);blaze_sound(w,BLAZE_DED);w->dying=1;s[0x2aa8/4]=24;}
    if(!blaze_charge_update(w))return boundary(error,capacity,"Charge effect kind/draw capacity is not supported");
    if(!blaze_marker_update(w))return boundary(error,capacity,"MARKER RAY target/draw capacity exceeded");
    if(!blaze_damage_effects(w))return boundary(error,capacity,"Damage effect draw capacity exceeded");
    return 1;
}
static void present(BlazeWorld *w,BlazeFrame *frame,const BlazeActorDraw *d,const uint8_t *rop) {
    const BlazeArchive *a;unsigned base;
    if(d->caption) {
        static const char *captions[]={"","TARGET","WEAK POINT","TYPE 00 ERISU"};
        char number[32];const char *str=d->caption<4?captions[d->caption]:w->loadout->boss_music[w->level->stage];
        if(d->caption==3 && w->level->stage==1)str="TYPE 01 FERIA";
        if(d->caption==3 && w->level->stage==2)str="TYPE 02 SESIRU";
        if(d->caption==3 && w->level->stage==3)str="TYPE 03 ISABEL";
        if(d->caption==3 && w->level->stage==4)str="TYPE 04 RAIYA";
        if(d->caption==6)str="LASER&";
        size_t i,n;int x=d->x,y=d->y,width;
        if(d->caption==5) {snprintf(number,sizeof(number),"%d",blaze_signed(d->characters));str=number;}
        a=d->sprite==0?w->font:&w->level->objects;base=d->sprite==0?0:0x40;
        width=a->sprites[d->sprite-base].width;n=strlen(str);
        if(d->alignment&2)x+=320-(int)n*width;
        if(d->alignment&4)x+=160-(int)n*width/2;
        if(d->alignment&0x20)y+=112;
        if(d->caption!=5 && n>d->characters)n=d->characters;
        for(i=0;i<n;++i)if(str[i]>=32 && str[i]<=90)blaze_draw(frame,a,d->sprite-base+(unsigned)str[i]-32,x+(int)i*width,y,NULL);
        return;
    }
    if(d->sprite==UINT32_MAX-2) {
        int x=d->x<0?0:d->x,y=d->y<0?0:d->y,right=d->x+d->pivot_x,bottom=d->y+d->pivot_y;
        if(right>320)right=320;if(bottom>240)bottom=240;
        for(;y<bottom;++y)if(x<right)memset(frame->pixels+y*320+x,d->palette,(size_t)(right-x));return;
    }
    if(d->sprite>=UINT32_MAX-1) {
        char label[40];
        if(d->sprite==UINT32_MAX)snprintf(label,sizeof(label),"%d",d->palette);
        else snprintf(label,sizeof(label),"%s",w->loadout->weapons[d->palette].name);
        if(d->sprite==UINT32_MAX) {
            size_t j;int width=blaze_level_sprite(w->level,0xf7)->width;
            for(j=0;label[j];++j)if(label[j]>=32 && label[j]<=90)
                blaze_draw(frame,&w->level->objects,0xf7-0x40+(unsigned)label[j]-32,
                    d->x+(int)j*width,d->y,rop);
        } else blaze_text(frame,w->font,label,d->x,d->y,0,rop);
        return;
    }
    a=d->sprite>=0x170?&w->level->tiles:&w->level->objects;base=d->sprite>=0x170?0x170:0x40;
    if(d->rotated) {
        blaze_draw_rotated(frame,a,d->sprite-base,d->x,d->y,d->pivot_x,d->pivot_y,d->angle,d->scale);
        return;
    }
    if(d->wave_age) {
        blaze_draw_wave(frame,a,d->sprite-base,d->x,d->y,d->wave_age-1,
            d->palette>=0?w->colors[d->palette]:NULL);return;
    }
    blaze_draw_scaled(frame,a,d->sprite-base,d->x,d->y,d->scale,d->height_scale<0?0:d->height_scale?d->height_scale:d->scale,
        d->blend?rop:NULL,d->palette>=3?w->backdrop_colors[d->palette-3]:d->palette>=0?w->colors[d->palette]:NULL);
}
static void map_layer(BlazeWorld *w,BlazeFrame *frame,unsigned layer,int x,int y,const uint8_t *colors) {
    int tx=x/32,ty=y/32,ix,iy;
    for(ix=tx;ix<tx+11;++ix)for(iy=ty;iy<ty+9;++iy) {
        uint32_t tile;
        if(ix<0 || iy<0 || (size_t)ix>=w->level->width[layer] || (size_t)iy>=w->level->height[layer])continue;
        tile=w->level->map[layer][(size_t)iy*w->level->width[layer]+(size_t)ix];
        if(blaze_signed(tile)>0) {
            if(colors)blaze_draw_scaled(frame,&w->level->tiles,tile,ix*32-x,iy*32-y,256,256,NULL,colors);
            else blaze_draw(frame,&w->level->tiles,tile,ix*32-x,iy*32-y,NULL);
        }
    }
}
/* 0x416050 prefix: the equipment icon is top-left anchored at the mount,
   scaled to half size, then covered by the hull where they overlap. */
void blaze_player_draw_mount(const BlazeWorld *w,BlazeFrame *frame) {
    const BlazeActorDraw *d=&w->mount_draw;
    if(d->sprite)blaze_draw_scaled(frame,&w->level->objects,d->sprite-0x40,
        d->x,d->y,d->scale,d->scale,NULL,NULL);
}
void blaze_world_draw(BlazeWorld *w,BlazeFrame *frame,const uint8_t *rop) {
    uint32_t *s=w->player.words;int bank=blaze_signed(s[0x74/4]);unsigned sprite=s[0];size_t i;
    memset(frame->pixels,0,sizeof(frame->pixels));
    if(w->backdrop_phase==18)blaze_world_backdrop_draw(w,frame);
    else {
        map_layer(w,frame,2,w->camera_x,w->camera_y,w->backdrop_phase?w->backdrop_colors[w->backdrop_phase/3]:NULL);
        map_layer(w,frame,1,w->camera_x*4/3,w->camera_y*4/3,NULL);
    }
    for(i=0;i<w->actor_pass0_count;++i)present(w,frame,&w->actor_draws[i],rop);
    /* The bounded draw list retains update-time positions and retirement frames. */
    for(i=0;i<w->effect_primary_count;++i)present(w,frame,&w->effect_draws[i],rop);
    for(i=0;i<w->primary_draw_count;++i) {
        const BlazeRotatedDraw *d=&w->shot_draws[i];
        blaze_draw_rotated(frame,&w->level->objects,d->sprite-0x40,d->x,d->y,
            d->pivot_x,d->pivot_y,d->angle,d->scale);
    }
    for(i=w->effect_primary_count;i<w->effect_weapon_count;++i)present(w,frame,&w->effect_draws[i],rop);
    blaze_player_draw_mount(w,frame);
    if(w->loadout->ship<2) {
        if(w->tick&3) {
            int target=blaze_signed(s[10])<0?0:blaze_signed(s[10])>0?6:3;
            bank+=(bank<target)-(bank>target);
        }
        s[0x74/4]=(uint32_t)bank;sprite+=(unsigned)bank;
    } else sprite+=s[8]<320?5:(w->tick/2)%3;
    if(sprite>=0x40)blaze_draw(frame,&w->level->objects,sprite-0x40,
        blaze_signed(s[5])-w->camera_x,blaze_signed(s[6])-w->camera_y,NULL);
    for(i=w->effect_weapon_count;i<w->effect_player_count;++i)present(w,frame,&w->effect_draws[i],rop);
    for(i=w->effect_player_count;i<w->effect_thunder_count;++i)present(w,frame,&w->effect_draws[i],rop);
    for(i=w->primary_draw_count;i<w->shot_draw_count;++i) {
        const BlazeRotatedDraw *d=&w->shot_draws[i];
        blaze_draw_rotated(frame,&w->level->objects,d->sprite-0x40,d->x,d->y,
            d->pivot_x,d->pivot_y,d->angle,d->scale);
    }
    for(i=w->effect_thunder_count;i<w->effect_gbraster_count;++i)present(w,frame,&w->effect_draws[i],rop);
    for(i=w->actor_pass0_count;i<w->actor_draw_count;++i) {
        const BlazeActorDraw *d=&w->actor_draws[i];
        present(w,frame,d,rop);
    }
    if(!w->backdrop_phase)map_layer(w,frame,0,w->camera_x*2,w->camera_y*2,NULL);
    for(i=w->effect_gbraster_count;i<w->effect_draw_count;++i)present(w,frame,&w->effect_draws[i],rop);
    blaze_draw(frame,&w->level->objects,0x14a-0x40,24,4,NULL);
    ++s[0x2aa8/4];
    {
        uint32_t health=s[0x2ad0/4];
        if(s[0x2aa8/4]<16 && (s[0x2aa8/4]&1))health-=s[0x2ad4/4];
        blaze_draw_scaled(frame,&w->level->objects,0x147-0x40,32,4,
            (int)((uint64_t)health*256/s[0x2ae4/4]),256,NULL,NULL);
    }
    if(!s[0x2adc/4] && !s[0x68/4])s[0x2ae0/4]=0;
    {
        unsigned stock;
        for(stock=0;stock<s[0x5c/4];++stock)
            blaze_draw(frame,&w->level->objects,0x71+(w->tick%6)-0x40,(int)stock*16,224,NULL);
    }
    if(w->loadout->mode!=2) {
        int x=blaze_signed(s[5])-w->camera_x,y=blaze_signed(s[6])-w->camera_y+32;
        int length=blaze_signed(s[0x2ad8/4])/8,j;
        for(j=0;j<length;++j)if(x+j>=0 && x+j<320 && y>=0 && y<240)
            frame->pixels[y*320+x+j]=(uint8_t)((w->tick&1)?250:254);
    }
    (void)rop;
}
