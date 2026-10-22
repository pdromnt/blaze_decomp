#include "blaze.h"
#include <string.h>

/* 0x416670/0x416d10. These are the original contiguous pool extents, also
   consumed by 0x407740. They remain within the player's single allocation. */
const BlazePool blaze_player_pools[]={
    {0x42d4,1,0x34,0xa4,-3},{0x2ae8,1,0x34,0x6f,-3},
    {0x3fb4,1,0x34,0xa1,32},{0x15bc,1,0x34,0x49,-3},
    {0x1588,1,0x34,0x4f,-3},{0x2da8,1,0x34,0x14c,-3},
    {0x2a68,1,0x34,0xab,-3},{0x3ff0,1,0x34,0x8f,-3},
    {0x17f4,1,0x34,0x16a,-3},{0x01f0,1,0x34,0xbd,-3},
    {0x4228,1,0x34,0x87,-3},{0x0a6c,3,0x3c,0x99,-3},
    {0x00ec,5,0x34,0x91,-3},{0x347c,16,0x34,0x85,2},
    {0x3e3c,2,0xbc,0x88,-3},{0x4368,32,0x34,0x59,-6},
    {0x0b20,48,0x34,0x41,-4},{0x14e0,3,0x38,0x60,-8},
    {0x1888,20,0x40,0x81,-3},{0x2ba0,10,0x34,0xa7,-3},
    {0x1d88,8,0x34,0x65,-3},{0x1f28,24,0x78,0x93,-8},
    {0x37bc,32,0x34,0x45,-3},{0x2ddc,8,0xd4,0x51,-3}
};
const size_t blaze_player_pool_count=sizeof(blaze_player_pools)/sizeof(blaze_player_pools[0]);

int32_t blaze_signed(uint32_t value) {
    return value<=INT32_MAX ? (int32_t)value : -1-(int32_t)(UINT32_MAX-value);
}
const BlazeSprite *blaze_level_sprite(const BlazeLevel *level,unsigned id) {
    if (id>=0x170 && id-0x170<level->tiles.count) return &level->tiles.sprites[id-0x170];
    if (id>=0x40 && id-0x40<level->objects.count) return &level->objects.sprites[id-0x40];
    return NULL;
}
static void record_reset(uint32_t *record,const BlazeSprite *sprite,unsigned id,int expansion) {
    record[0]=id; record[3]=record[4]=record[5]=record[6]=0;
    record[7]=sprite->width; record[8]=sprite->visible_height;
    record[12]=(uint32_t)expansion;
}
/* 0x415f50, including the original three-slot CHANGE WEAPON behavior.
   Power comes from config, rather than a duplicate serialized world record. */
void blaze_player_weapon(BlazePlayer *p,const BlazeLoadout *loadout,unsigned weapon) {
    uint32_t *s=p->words; unsigned slot;
    s[0xb0/4]=weapon; s[0x2adc/4]=0;
    s[0x2ad8/4]=(uint32_t)loadout->weapons[weapon].power;
    s[0xe8/4]=(uint32_t)loadout->weapons[weapon].action;
    if (loadout->mode!=3) return;
    for (slot=0;slot<3;++slot) if (s[0x40/4+slot]==weapon) break;
    if (slot<3 && weapon) {
        s[0x40/4+slot]=weapon; s[0x4c/4+slot]=(uint32_t)loadout->weapons[weapon].power;
        s[0x3c/4]=slot; return;
    }
    for (slot=0;slot<3;++slot) if (!s[0x40/4+slot]) break;
    if (slot<3 && weapon) s[0x3c/4]=slot;
    slot=s[0x3c/4];
    s[0x40/4+slot]=weapon; s[0x4c/4+slot]=(uint32_t)loadout->weapons[weapon].power;
}
static int reset_player(BlazePlayer *p,const BlazeLevel *level,const BlazeArchive *font,const BlazeLoadout *loadout,unsigned ship,unsigned weapon) {
    static const unsigned ship_sprite[]={0xe2,0xe9,0xf0};
    static const int expansion[]={-8,-6,-20};
    static const unsigned zero_offsets[]={0xa4,0x80,0xcc,0xd0,0xc8,0xc4,0xd4,0xd8,0xdc,
        0x64,0x68,0x78,0xa0,0x34,0x94,0x24,0x28};
    const BlazeSprite *sprite; size_t i,j; uint32_t *s=p->words;
    if (ship>2 || loadout->mode>4 || weapon>10 || s[0x3c/4]>2) return 0;
    sprite=blaze_level_sprite(level,ship_sprite[ship]); if (!sprite) return 0;
    if (font->count<=0x15) return 0;
    for (i=0;i<blaze_player_pool_count;++i)
        if (!blaze_level_sprite(level,blaze_player_pools[i].sprite)) return 0;
    s[0xac/4]=ship; s[0xb0/4]=weapon;
    record_reset(s,sprite,ship_sprite[ship],expansion[ship]);
    s[6]=160; s[8]=160+sprite->visible_height;
    for(i=0;i<sizeof(zero_offsets)/sizeof(zero_offsets[0]);++i) s[zero_offsets[i]/4]=0;
    s[0x2c/4]=64; s[0x98/4]=s[0x9c/4]=64; s[4]=1;
    s[0x5c/4]+=1; s[0x74/4]=2;
    /* 0x413710 uses the persistent font sprite table (id 0x15). */
    record_reset(s+0x2a9c/4,&font->sprites[0x15],0x15,-3);
    s[0x2ad4/4]=0; s[0x2ad0/4]=s[0x2ae4/4]; s[0x2adc/4]=0;
    blaze_player_weapon(p,loadout,weapon);
    for (i=0;i<blaze_player_pool_count;++i) {
        const BlazePool *pool=&blaze_player_pools[i];
        sprite=blaze_level_sprite(level,pool->sprite);
        for(j=0;j<pool->count;++j) record_reset(s+(pool->offset+j*pool->stride)/4,sprite,pool->sprite,pool->expansion);
    }
    return 1;
}
int blaze_player_reset(BlazePlayer *p,const BlazeLevel *level,const BlazeArchive *font,const BlazeLoadout *loadout) {
    return reset_player(p,level,font,loadout,loadout->ship,loadout->weapon);
}
int blaze_player_restart(BlazePlayer *p,const BlazeLevel *level,const BlazeArchive *font,const BlazeLoadout *loadout) {
    return reset_player(p,level,font,loadout,p->words[0xac/4],p->words[0xb0/4]);
}
void blaze_player_clear(BlazePlayer *player) { memset(player,0,sizeof(*player)); }

static void translate(uint32_t *s) { s[5]+=s[9];s[7]+=s[9];s[6]+=s[10];s[8]+=s[10]; }
static void constrain(uint32_t *s,const BlazeSprite *sprite,int32_t camera_x) {
    if(blaze_signed(s[5])<camera_x) {s[5]=(uint32_t)camera_x;s[7]=s[5]+sprite->width;}
    if(blaze_signed(s[7])>=camera_x+320) {s[7]=(uint32_t)(camera_x+320);s[5]=s[7]-sprite->width;}
    if(blaze_signed(s[6])<0) {s[6]=0;s[8]=sprite->visible_height;}
    if(blaze_signed(s[8])>=320) {s[8]=320;s[6]=320-sprite->visible_height;}
}
/* 0x416290: original integer table/division rules, including type-2 jump
   acceleration. This is direct C arithmetic, not software x87 execution. */
void blaze_player_move(BlazePlayer *p,const BlazeLevel *level,const int32_t trig[320],unsigned direction,int32_t camera_x) {
    uint32_t *s=p->words; unsigned ship=s[0xac/4];
    const BlazeSprite *sprite=blaze_level_sprite(level,s[0]);
    s[0xe4/4]=direction;
    memcpy(s+0x84/4,s+5,16);
    if(ship<2) {
        int divisor=ship==0?40:32;
        s[9]=(uint32_t)(trig[direction&255]/divisor);
        s[10]=(uint32_t)(-trig[(direction&255)+64]/divisor);
        translate(s);
    } else if(ship==2) {
        int upward=direction<64 || (direction>192 && direction<257);
        if(blaze_signed(s[8])>=320 && upward) {s[10]=(uint32_t)-16;s[0x78/4]=0;}
        if(upward) {
            s[0x78/4]-=8;
            if(blaze_signed(s[0x78/4])>0) {
                if(blaze_signed(s[10])>-8) s[10]-=3;
                s[0x7c/4]=1;
            }
        }
        if(direction>0 && direction<128) {s[5]+=8;s[7]+=8;}
        if(direction>128 && direction<256) {s[5]-=8;s[7]-=8;}
    }
    constrain(s,sprite,camera_x);memcpy(s+0x84/4,s+5,16);
}
