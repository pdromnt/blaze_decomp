#include "blaze.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); goto fail; } } while(0)
static int press(BlazeApp *a,unsigned mask) {
    return blaze_app_tick(a,0) && blaze_app_tick(a,mask);
}
/* Inject report entry only; resource loading and world/player restart are real. */
static int report_next_fixture(BlazeApp *a) {
    a->state=BLAZE_STAGE_CLEAR;a->tick=128;a->world.enemies_created=1;a->world.player.words[0x94/4]=1;
    if(!blaze_app_tick(a,0))return 0;
    while(a->state==BLAZE_LOADING)if(!blaze_app_tick(a,0))return 0;
    return a->state==BLAZE_START;
}
int main(int argc,char **argv) {
    static const char *archives[]={"BLAZE.DAR","BLZ_END.DAR","BLZ_MAP1.DAR","BLZ_MAP2.DAR",
        "BLZ_MAP3.DAR","BLZ_MAP4.DAR","BLZ_MAP5.DAR","blz_opn.dar","BLZ_SEL.DAR","BLZ_TEA.DAR","BLZ_TEXT.DAR"};
    BlazeApp *a=calloc(1,sizeof(*a)); BlazeArchive archive={0};
    char path[1200],name[80],error[256]; size_t i,j; int cycle;
    CHECK(argc==3 && a);
    CHECK(blaze_app_open(a,argv[1],argv[2]));CHECK(a->frame_ms==40);
    blaze_random_begin(&a->random,1); /* Reproducible gameplay fixture, not wall-clock RNG. */
    /* Every shipped DAR must parse, including short transparent rows and crops. */
    for (i=0;i<sizeof(archives)/sizeof(archives[0]);++i) {
        uint8_t *copy; size_t size;
        snprintf(name,sizeof(name),"dar/%s",archives[i]);
        CHECK(blaze_path(path,sizeof(path),argv[1],name));
        CHECK(blaze_archive_read(&archive,path,error,sizeof(error)));
        for (j=0;j<archive.count;++j) blaze_draw(&a->frame,&archive,j,-7,-3,NULL);
        CHECK(blaze_file_read(path,&copy,&size));
        /* Failed replacement keeps the previous archive alive. */
        CHECK(!blaze_archive_decode(&archive,copy,size-1,error,sizeof(error)));
        CHECK(archive.bytes && archive.count);
        /* Corrupt the first run to prove rejection before rendering. */
        memset(copy+archive.sprites[0].pixels,0,4);
        CHECK(!blaze_archive_decode(&archive,copy,size,error,sizeof(error)));
        free(copy); blaze_archive_free(&archive);
    }
    for (i=0;i<40;++i) CHECK(blaze_app_tick(a,0));
    CHECK(a->state==BLAZE_TITLE);
    /* Repeat real menu loads/unloads, both cancel and confirm paths. */
    for (cycle=0;cycle<30;++cycle) {
        CHECK(press(a,BLAZE_CONFIRM)); CHECK(a->state==BLAZE_SELECT && a->select_step==0);
        CHECK(press(a,BLAZE_DOWN)); CHECK(a->ship==1);
        CHECK(blaze_app_tick(a,BLAZE_DOWN)); CHECK(a->ship==1); /* held mask suppression */
        CHECK(press(a,BLAZE_CONFIRM)); CHECK(a->select_step==1);
        CHECK(press(a,BLAZE_RIGHT)); CHECK(a->mode==1);
        CHECK(press(a,BLAZE_CONFIRM)); CHECK(a->select_step==2);
        CHECK(press(a,BLAZE_CANCEL)); CHECK(a->state==BLAZE_TITLE);
        for (i=0;i<5;++i) CHECK(blaze_app_tick(a,0));
    }
    CHECK(press(a,BLAZE_DOWN)); CHECK(a->title_item==1);
    CHECK(press(a,BLAZE_CONFIRM)); CHECK(a->state==BLAZE_SELECT && a->mode==4 && a->select_step==2);
    CHECK(press(a,BLAZE_CANCEL));
    for (i=0;i<5;++i) CHECK(blaze_app_tick(a,0));
    CHECK(press(a,BLAZE_DOWN)); CHECK(a->title_item==2);
    CHECK(press(a,BLAZE_CONFIRM)); CHECK(a->state==BLAZE_OPTIONS);
    CHECK(press(a,BLAZE_CANCEL)); CHECK(a->state==BLAZE_TITLE);
    for (i=0;i<5;++i) CHECK(blaze_app_tick(a,0));
    CHECK(press(a,BLAZE_UP)); CHECK(press(a,BLAZE_UP));
    CHECK(press(a,BLAZE_CONFIRM));
    CHECK(press(a,BLAZE_CONFIRM)); CHECK(press(a,BLAZE_CONFIRM));
    CHECK(press(a,BLAZE_CONFIRM)); CHECK(a->state==BLAZE_LOADING && a->world.active);
    CHECK(a->level_data.map[0] && a->level_data.tiles.count==327);
    CHECK(a->level_data.spawns[0].x==1056 && a->level_data.spawns[0].y==96);
    CHECK(a->world.player.words[0x2ad0/4]==192);
    for(i=0;i<4;++i)CHECK(blaze_app_tick(a,0));
    CHECK(a->state==BLAZE_START);
    for(i=0;i<16;++i)CHECK(blaze_app_tick(a,BLAZE_UP|BLAZE_RIGHT));
    CHECK(blaze_signed(a->world.player.words[5])-a->world.camera_x>0);
    CHECK(a->world.player.words[0xe4/4]==32);
    CHECK(press(a,BLAZE_ABORT));CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.map[0]);
    /* Construct -> update/draw -> abort -> release -> reconstruct through menus. */
    for(cycle=0;cycle<12;++cycle) {
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<(size_t)cycle%3;++i)CHECK(press(a,BLAZE_DOWN));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        CHECK(a->state==BLAZE_LOADING && a->world.active);
        for(i=0;i<4;++i)CHECK(blaze_app_tick(a,0));
        for(i=0;i<38;++i)CHECK(blaze_app_tick(a,i<16?BLAZE_RIGHT:0));
        CHECK(a->state==BLAZE_GAME);
        /* Abort a live native flyer and active primary shots, then reconstruct
           through the menus. This catches retained registries/pool callbacks. */
        for(i=0;i<256 && !a->world.enemies_created;++i)CHECK(blaze_app_tick(a,0));
        CHECK(a->world.enemies_created && a->world.actors[0][4]);
        CHECK(press(a,BLAZE_CONFIRM));
        CHECK(a->world.player.words[(0xb20+0x10)/4]);
        {unsigned live=0,k;for(k=0;k<48;++k)live+=a->world.player.words[(0xb20+k*0x34+0x10)/4]!=0;
         CHECK(live==(a->loadout.ship==2?5u:3u));}
        CHECK(press(a,BLAZE_ABORT));CHECK(!a->world.active && !a->level_data.objects.bytes);
        CHECK(!a->world.actor_draw_count && !a->world.shot_draw_count && !a->world.enemies_created);
        CHECK(!blaze_world_step(&a->world,a->trig,0,error,sizeof(error)));
    }
    /* Real control sequence, all ships: charge/hold/release, X movement,
       double tap, stock, active-combat abort and reconstruction. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned saw_jet=0,saw_secondary=0,saw_radial=0,saw_stock=0;
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<(unsigned)cycle%3;++i)CHECK(press(a,BLAZE_DOWN));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<240;++i) {
            unsigned input=0,t=(unsigned)i;
            if(t>=128 && t<170)input=BLAZE_CONFIRM;
            if(t==172 || t==175)input=BLAZE_CANCEL|BLAZE_UP;
            if(t==184)input=BLAZE_CONFIRM;
            if(t==185 || t==186)input=BLAZE_CONFIRM|BLAZE_CANCEL;
            if(t>=220 && t<240)input=BLAZE_CANCEL|BLAZE_DOWN;
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED);
            saw_jet|=a->world.player.words[(0x1588+0x10)/4];
            saw_secondary|=a->world.player.words[0x4238/4];
            saw_radial|=a->world.player.words[(0x347c+0x10)/4];
            saw_stock|=a->world.player.words[0x4000/4];
        }
        CHECK(saw_jet && saw_secondary && saw_radial && saw_stock);
        CHECK(a->world.player.words[0x5c/4]==0);
        CHECK(a->world.player.words[0x4238/4]);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->world.effect_draw_count);
        for(i=0;i<BLAZE_PLAYER_BYTES/4;++i)CHECK(!a->world.player.words[i]);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.bytes);
    }
    /* Bounded fixture supplies the session-unlocked prerequisite; actual
       pickup consumption equips LIGHTNING (disabled pickups reroll originally).
       FIRE STORM is selected in the normal menu. Each ship exercises both. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned saw_charge=0,saw_pulse=0,saw_fire=0,lightning=(unsigned)cycle&1;
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<(unsigned)cycle%3;++i)CHECK(press(a,BLAZE_DOWN));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        CHECK(press(a,BLAZE_DOWN));CHECK(a->weapon==2);
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<240;++i) {
            unsigned input=(i>=128 && i<176) || i>=220?BLAZE_CONFIRM:0;
            if(lightning && i==120) {
                a->loadout.weapons[3].enabled=1;
                CHECK(blaze_actor_spawn(&a->world,a->world.player.words+5,11,3,0));
            }
            if(lightning && i==239)input=0; /* Abort with the released pulse live. */
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED);
            if(lightning && i>=122)CHECK(a->world.player.words[0xb0/4]==3);
            saw_pulse|=a->world.player.words[(0x2da8+0x10)/4];
            for(j=0;j<10;++j)saw_fire|=a->world.player.words[(0x2ba0+j*0x34+0x10)/4];
            for(j=a->world.effect_player_count;j<a->world.effect_draw_count;++j)
                saw_charge|=a->world.effect_draws[j].sprite>=0xb8 && a->world.effect_draws[j].sprite<=0xbb;
        }
        CHECK(lightning?(saw_charge && saw_pulse && a->world.player.words[(0x2da8+0x10)/4]):saw_fire);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->world.effect_draw_count);
        for(i=0;i<BLAZE_PLAYER_BYTES/4;++i)CHECK(!a->world.player.words[i]);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.bytes);
    }
    /* NOVA WAVE is available in the preserved menu settings. The S-LASER
       fixture supplies its session unlock, then consumes a real pickup while
       NOVA trails are live: cooldown must delay the following weapon. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned saw_nova=0,saw_laser=0,saw_charge=0,laser=(unsigned)cycle&1;
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<(unsigned)cycle%3;++i)CHECK(press(a,BLAZE_DOWN));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<3;++i)CHECK(press(a,BLAZE_DOWN));CHECK(a->weapon==4);
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<240;++i) {
            unsigned input=(i>=128 && i<160) || (i>=200 && i<215) || (i>=220 && i<239)?BLAZE_CONFIRM:0;
            if(laser && i==161) {
                a->loadout.weapons[5].enabled=1;
                CHECK(blaze_actor_spawn(&a->world,a->world.player.words+5,11,5,0));
            }
            if(laser && i>=162)input=BLAZE_CONFIRM;
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED);
            if(laser && i>=163)CHECK(a->world.player.words[0xb0/4]==5);
            if(laser && i==175)CHECK(a->world.effect_primary_count<120 && a->world.player.words[0x68/4]);
            saw_nova|=a->world.player.words[0x3e4c/4] && a->world.player.words[0x3f08/4];
            saw_laser|=a->world.effect_primary_count>=120;
            for(j=a->world.effect_player_count;j<a->world.effect_draw_count;++j)
                saw_charge|=a->world.effect_draws[j].sprite>=0xd9 && a->world.effect_draws[j].sprite<=0xdc;
        }
        CHECK(saw_charge && saw_nova);
        CHECK(laser?(saw_laser && a->world.effect_primary_count>=120):
            (a->world.player.words[0x3e4c/4] && a->world.player.words[0x3f08/4]));
        CHECK(blaze_app_tick(a,BLAZE_ABORT));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->world.effect_draw_count);
        for(i=0;i<BLAZE_PLAYER_BYTES/4;++i)CHECK(!a->world.player.words[i]);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.bytes);
    }
    /* Bounded THUNDER fixture: real pickup/enemy construction, locked release,
       retained-handle restart into inactive reset slots, then a live-bolt abort. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned saw_lock=0,saw_bolt=0,saw_hits=0;
        blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<(unsigned)cycle%3;++i)CHECK(press(a,BLAZE_DOWN));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<213;++i) {
            unsigned input=(i>=128 && i<136) || (i>=160 && i<176) || (i>=200 && i<208)?BLAZE_CONFIRM:0;
            if(i==120) {a->loadout.weapons[6].enabled=1;CHECK(blaze_actor_spawn(&a->world,a->world.player.words+5,11,6,0));}
            if(i==135 || i==167 || i==207) {
                uint32_t r[4];memcpy(r,a->world.player.words+0xb4/4,16);
                r[0]+=118;r[2]+=118;r[1]+=8;r[3]+=8;
                CHECK(blaze_actor_spawn(&a->world,r,1,192,0));
            }
            if(i==168) {
                CHECK(a->world.player.words[(0x42d4+0x34)/4]);
                CHECK(blaze_world_restart(&a->world));
                CHECK(a->world.player.words[(0x42d4+0x34)/4]);
            }
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED);
            if(i>=122)CHECK(a->world.player.words[0xb0/4]==6);
            if(i==176)for(j=0;j<8;++j)CHECK(!a->world.player.words[(0x42d4+0x34)/4+j]);
            for(j=0;j<8;++j)saw_lock|=a->world.player.words[(0x42d4+0x34)/4+j];
            saw_bolt|=a->world.player.words[(0x1f0+0x10)/4];
            saw_hits|=a->world.player.words[(0x1f0+0x878)/4];
        }
        CHECK(saw_lock && saw_bolt && saw_hits && a->world.player.words[(0x1f0+0x10)/4]);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->world.effect_draw_count);
        for(i=0;i<BLAZE_PLAYER_BYTES/4;++i)CHECK(!a->world.player.words[i]);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.bytes);
    }
    /* Explicit unlock, real pickup and anchored pulse hits/fade/retirement,
       then reuse and abort with the next pulse live, for every ship twice. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned saw_charge=0,saw_pulse=0,saw_hit=0,saw_fade=0,saw_retired=0;
        blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<(unsigned)cycle%3;++i)CHECK(press(a,BLAZE_DOWN));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<260;++i) {
            unsigned input=(i>=128 && i<160) || (i>=220 && i<252)?BLAZE_CONFIRM:0;
            if(i==120) {a->loadout.weapons[7].enabled=1;CHECK(blaze_actor_spawn(&a->world,a->world.player.words+5,11,7,0));}
            if(i==159 || i==251) {
                uint32_t r[4];memcpy(r,a->world.player.words+5,16);r[0]+=112;r[2]+=112;
                CHECK(blaze_actor_spawn(&a->world,r,1,192,0));
            }
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED);
            if(i>=122)CHECK(a->world.player.words[0xb0/4]==7);
            for(j=0;j<256;++j)saw_hit|=a->world.actors[j][0x64/4]==22;
            for(j=0;j<3;++j) {
                uint32_t *pulse=a->world.player.words+(0x14e0+j*0x38)/4;
                saw_pulse|=pulse[4];saw_fade|=pulse[4] && pulse[3]>25;
                if(i==200)saw_retired|=!pulse[4] && pulse[3]==33;
            }
            for(j=a->world.effect_gbraster_count;j<a->world.effect_draw_count;++j)
                saw_charge|=a->world.effect_draws[j].sprite>=0x5d && a->world.effect_draws[j].sprite<=0x60;
        }
        CHECK(saw_charge && saw_pulse && saw_hit && saw_fade && saw_retired);
        CHECK(a->world.player.words[(0x14e0+0x10)/4]);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->world.effect_draw_count);
        for(i=0;i<BLAZE_PLAYER_BYTES/4;++i)CHECK(!a->world.player.words[i]);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.bytes);
    }
    /* All remaining selected weapons: real pickup after explicit unlock,
       native enemies/updates, repeat construction and owned live teardown. */
    for(cycle=0;cycle<18;++cycle) {
        unsigned weapon=8+(unsigned)cycle/6,saw_active=0,saw_hit=0,saw_retired=0,saw_drone=0;
        unsigned offset=weapon==8?0x1f28:weapon==9?0x2a68:0x3fb4;
        blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<(unsigned)cycle%3;++i)CHECK(press(a,BLAZE_DOWN));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<260;++i) {
            unsigned input=(i>=128 && i<192) || (i>=(weapon==8?240u:248u) && i<259)?BLAZE_CONFIRM:0;
            if(i==120) {a->loadout.weapons[weapon].enabled=1;CHECK(blaze_actor_spawn(&a->world,a->world.player.words+5,11,weapon,0));}
            if(i==134 || i==150 || i==(weapon==8?246u:254u)) {
                uint32_t r[4];memcpy(r,a->world.player.words+0xb4/4,16);
                r[0]+=weapon==8?80:16;r[2]+=weapon==8?80:16;
                if(weapon==9) {r[1]+=16;r[3]+=16;}
                CHECK(blaze_actor_spawn(&a->world,r,weapon==9?8:2,192,0));
            }
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED);
            if(i>=122)CHECK(a->world.player.words[0xb0/4]==weapon);
            saw_active|=a->world.player.words[(offset+0x10)/4];
            if(i==230)saw_retired=!a->world.player.words[(offset+0x10)/4];
            /* These enemies update in pass 1, after the slash. The original
               flash decay subtracts four on every third world tick. */
            for(j=0;j<256;++j)saw_hit|=a->world.actors[j][0x64/4]==
                (weapon==8?10u:weapon==9?(a->world.tick%3==0?94u:98u):3u);
            for(j=0;j<3;++j)saw_drone|=a->world.player.words[(0xa6c+j*0x3c+0x10)/4];
        }
        if(!(saw_active && saw_hit && saw_retired))fprintf(stderr,"Lifecycle weapon=%u cycle=%u active=%u hit=%u retired=%u drone=%u\n",weapon,(unsigned)cycle,saw_active,saw_hit,saw_retired,saw_drone);
        CHECK(saw_active && saw_hit && saw_retired);if(weapon==10)CHECK(saw_drone);
        if(!a->world.player.words[(offset+0x10)/4])fprintf(stderr,"Live final weapon=%u cycle=%u active=%u hit=%u retired=%u drone=%u age=%u\n",weapon,(unsigned)cycle,saw_active,saw_hit,saw_retired,saw_drone,a->world.player.words[(offset+0xc)/4]);
        CHECK(a->world.player.words[(offset+0x10)/4]);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->world.effect_draw_count);
        for(i=0;i<BLAZE_PLAYER_BYTES/4;++i)CHECK(!a->world.player.words[i]);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.bytes);
    }
    for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
    CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
    CHECK(press(a,BLAZE_CONFIRM|BLAZE_CANCEL));
    CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.map[0]);
    for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
    CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
    for(i=0;i<512 && a->world.player.words[0x2ad0/4]==192;++i)CHECK(blaze_app_tick(a,0));
    CHECK(a->state==BLAZE_GAME && a->world.player.words[0x2ad0/4]==182);
    CHECK(a->level_data.spawns[6].parameter==22 && !a->level_data.spawns[6].flags && !a->world.actors[0][4]);
    CHECK(a->world.player.words[(0x1888+0x10)/4]);
    /* Unsupported registration must keep the original pending spawn intact. */
    a->level_data.spawns[0].parameter=10;a->level_data.spawns[0].x=0;a->level_data.spawns[0].flags=1;
    CHECK(!blaze_world_step(&a->world,a->trig,0,error,sizeof(error)));
    CHECK(strstr(error,"Actor type 10") && a->level_data.spawns[0].flags==1);
    a->level_data.spawns[0].parameter=18;
    CHECK(!blaze_world_step(&a->world,a->trig,0,error,sizeof(error)));
    CHECK(strstr(error,"Actor type 18") && a->level_data.spawns[0].flags==1);
    CHECK(blaze_app_tick(a,BLAZE_ABORT)); /* Return through the real application owner. */
    CHECK(a->state==BLAZE_TITLE && !a->world.active);
    blaze_random_begin(&a->random,1);
    for(cycle=0;cycle<4;++cycle) {
        uint32_t before=a->random.state;BlazeRandom expected=a->random;
        /* Original Select loads only at tick 16 (0x4030f0). This fast
           input sequence leaves earlier, so only Openning consumes RNG. */
        blaze_random_next(&expected);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        CHECK(a->world.random==&a->random && a->random.state==expected.state);
        for(i=0;i<280 && !a->world.enemies_created;++i)CHECK(blaze_app_tick(a,0));
        CHECK(a->world.enemies_created);
        for(i=0;i<64 && !a->world.player.words[0x58/4];++i)CHECK(blaze_app_tick(a,(i%4)==0?BLAZE_CONFIRM:0));
        CHECK(a->state==BLAZE_GAME && a->world.player.words[0x58/4]>0 && !a->world.actors[0][4]);
        CHECK(a->world.shot_draw_count || a->world.player.words[(0xb20+0x10)/4]);
        before=a->random.state;
        CHECK(blaze_app_tick(a,BLAZE_ABORT));CHECK(a->state==BLAZE_TITLE && !a->world.active);
        CHECK(a->random.state==before && !a->world.random && !a->world.effect_draw_count);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.expanded);
    }
    /* Reach real type-1/type-0/type-8 registrations and their shots, then tear down the live
       family through the application owner, using the same diagnostic seed per route. */
    for(cycle=0;cycle<12;++cycle) {
        unsigned seen_enemy=0,seen_projectile=0;
        unsigned enemy_type=cycle<4?1:cycle<8?0:8,shot_type=cycle<4?43:cycle<8?20:16;
        unsigned frames=cycle<4?400:cycle<8?620:860,fire_until=cycle<4?260:cycle<8?430:620;
        blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<frames;++i) {
            CHECK(blaze_app_tick(a,i>=116 && i<fire_until && i%4==0?BLAZE_CONFIRM:0));
            CHECK(a->state!=BLAZE_BLOCKED);
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {
                seen_enemy|=a->world.actors[j][0x4c/4]==enemy_type;
                seen_projectile|=a->world.actors[j][0x4c/4]==shot_type;
            }
        }
        CHECK(seen_enemy && seen_projectile && a->state==BLAZE_GAME);
        {unsigned live=0;for(j=0;j<256;++j)live+=a->world.actors[j][4]!=0;CHECK(live);}
        CHECK(blaze_app_tick(a,BLAZE_ABORT));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->world.trig && !a->world.actor_draw_count);
        CHECK(!a->level_data.objects.bytes && !a->level_data.tiles.expanded);
    }
    /* Bounded state fixture: enter just before the real type-12 spawn, leaving
       its data/registration and following combat untouched. Retire this family
       through the application owner with parent, child and projectiles live. */
    for(cycle=0;cycle<4;++cycle) {
        unsigned seen_parent=0,seen_child=0,seen_shot=0;
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME);
        a->world.camera_x=9380;a->world.player.words[5]=9460;a->world.player.words[7]=9492;
        for(i=0;i<128;++i)if(a->level_data.spawns[i].x<9380)a->level_data.spawns[i].flags=0;
        CHECK(a->level_data.spawns[15].parameter==12 && a->level_data.spawns[15].flags);
        for(i=0;i<120;++i) {
            CHECK(blaze_app_tick(a,(i%4==0?BLAZE_CONFIRM:0)|(i<20?BLAZE_UP:0)));
            CHECK(a->state==BLAZE_GAME);
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {
                seen_parent|=a->world.actors[j][0x4c/4]==12;
                seen_child|=a->world.actors[j][0x4c/4]==21;
                seen_shot|=a->world.actors[j][0x4c/4]==3;
            }
        }
        CHECK(seen_parent && seen_child && seen_shot && !a->level_data.spawns[15].flags);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));CHECK(a->state==BLAZE_TITLE && !a->world.active);
        CHECK(!a->world.actor_draw_count && !a->level_data.tiles.bytes);
        for(i=0;i<256;++i)CHECK(!a->world.actors[i][4]);
    }
    /* Bounded boss fixture: actual spawn, active children, death, report and
       transactional next-stage resources, then abort before unsupported actors. */
    for(cycle=0;cycle<4;++cycle) {
        unsigned parent=256,seen_beam=0,seen_turret=0,seen_body=0;
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));a->loadout.mode=(cycle&1)?4:0;
        a->world.camera_x=16050;a->world.player.words[5]=16130;a->world.player.words[7]=16162;
        a->world.player.words[6]=0;a->world.player.words[8]=32;a->world.encounter_phase=18;
        for(i=0;i<128;++i)if(a->level_data.spawns[i].x<16050)a->level_data.spawns[i].flags=0;
        for(i=0;i<180;++i) {
            CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME);
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {
                if(a->world.actors[j][0x4c/4]==4)parent=(unsigned)j;
                seen_turret|=a->world.actors[j][0x4c/4]==5;seen_body|=a->world.actors[j][0x4c/4]==7;
                seen_beam|=a->world.actors[j][0x4c/4]==28;
            }
            if(parent<256 && i==25)a->world.actors[parent][0x58/4]=4; /* Exercise the actual laser phase. */
        }
        CHECK(parent<256 && seen_turret && seen_body && seen_beam);
        a->world.actors[parent][0x50/4]=UINT32_MAX;
        for(i=0;i<120 && a->state==BLAZE_GAME;++i)CHECK(blaze_app_tick(a,0));
        CHECK(a->state==BLAZE_STAGE_CLEAR && a->world.active && a->world.requested_screen==4);
        /* Actual ERISU death enters the report; bound the miss/locked state
           after its opening world flashes. Verify the rendered bottom-right
           notice on/off/on cadence and award, including ineligible routes. */
        while(a->tick<32)CHECK(blaze_app_tick(a,0));
        a->world.player.words[0x94/4]=cycle==1?1:0;
        a->loadout.weapons[3].enabled=cycle==2;
        for(i=0;i<3;++i) {
            BlazeFrame expected={0};unsigned lit=0,k;char notice[80];
            int eligible=cycle!=1 && cycle!=2;
            CHECK(blaze_app_tick(a,0));
            if(eligible && (i&1)) {
                snprintf(notice,sizeof(notice),"%s GET",a->loadout.weapons[3].name);
                blaze_text(&expected,&a->font,notice,0,224,2,a->rop);
            }
            CHECK(!memcmp(a->frame.pixels+224*320,expected.pixels+224*320,16*320));
            for(k=224*320;k<240*320;++k)lit+=a->frame.pixels[k]!=0;
            CHECK((lit!=0)==(eligible && (i&1)));
        }
        while(a->tick<65)CHECK(blaze_app_tick(a,0));
        CHECK(blaze_app_tick(a,BLAZE_CONFIRM));CHECK(a->state==BLAZE_LOADING && a->pending_stage==1 && a->restart_pending);
        CHECK(a->loadout.weapons[3].enabled==(cycle!=1));
        CHECK(blaze_app_tick(a,0));CHECK(blaze_app_tick(a,0));
        CHECK(a->level_data.stage==1 && !a->restart_pending && a->world.active && a->world.tick==0);
        for(i=0;i<256;++i)CHECK(!a->world.actors[i][4]);
        while(a->state==BLAZE_LOADING)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_START);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.tiles.bytes);
    }
    /* Real application death/continue/no/timeout and snapshot teardown routes. */
    for(cycle=0;cycle<9;++cycle) {
        uint32_t rect[4],before_rng;unsigned before_tick;int before_camera;
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));
        CHECK(a->state==BLAZE_GAME);
        a->loadout.mode=(cycle&1)?4:0;blaze_player_weapon(&a->world.player,&a->loadout,2);
        a->world.player.words[0x2ad0/4]=0;a->world.player.words[0x58/4]=777;a->world.player.words[0x5c/4]=3;
        rect[0]=(uint32_t)a->world.camera_x+280;rect[1]=0;rect[2]=rect[0]+32;rect[3]=32;
        CHECK(blaze_actor_create(&a->world,a->world.actors[0],rect,8,0,1,0)==1);
        CHECK(blaze_actor_attach(&a->world,1,-32,0,37,0,0));
        CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME_OVER && a->world.dying && a->tick==0);
        before_tick=a->world.tick;before_camera=a->world.camera_x;
        for(i=0;i<(cycle==7?8u:32u);++i)CHECK(blaze_app_tick(a,0));
        CHECK(a->world.camera_x==before_camera);
        if(cycle==7) {CHECK(blaze_app_tick(a,BLAZE_ABORT));CHECK(a->state==BLAZE_TITLE && !a->world.active);continue;}
        CHECK(a->world.tick==before_tick+16);
        CHECK(blaze_app_tick(a,0));CHECK(a->game_over_image.count==1 && a->game_over_image.bytes && a->world.camera_x==0);
        if(cycle==8) {CHECK(blaze_app_tick(a,BLAZE_ABORT));CHECK(a->state==BLAZE_TITLE && !a->game_over_image.bytes && !a->world.active);continue;}
        while(a->tick<(cycle==6?1280u:cycle<4?49u:48u))CHECK(blaze_app_tick(a,0));
        before_rng=a->random.state;
        if(cycle<4) {
            CHECK(blaze_app_tick(a,cycle==2?BLAZE_CONFIRM|BLAZE_CANCEL:BLAZE_CONFIRM));
            CHECK(a->state==BLAZE_LOADING && a->tick==1 && a->restart_pending && !a->game_over_image.bytes);
            CHECK(a->world.player.words[0x58/4]==0);
            CHECK(blaze_app_tick(a,0));CHECK(!a->restart_pending && !a->world.dying && a->world.tick==0);
            CHECK(a->random.state==before_rng && a->world.player.words[0xb0/4]==2);
            CHECK(a->world.player.words[0x5c/4]==((cycle&1)?1u:4u));
            CHECK(a->world.player.words[0x2ad0/4]==a->world.player.words[0x2ae4/4]);
            CHECK(a->level_data.spawns[0].flags && a->world.actor_cursor==0 && a->world.enemies_created==0);
            for(i=0;i<256;++i)CHECK(!a->world.actors[i][4]);
            for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME);
            for(i=0;i<230;++i)CHECK(blaze_app_tick(a,(i%4)==0?BLAZE_CONFIRM:0));
            CHECK(a->state==BLAZE_GAME && a->world.enemies_created && a->world.player.words[0x58/4]>0);
            CHECK(blaze_app_tick(a,BLAZE_ABORT));
        } else CHECK(blaze_app_tick(a,cycle==6?0:BLAZE_CANCEL));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.objects.bytes && !a->game_over_image.bytes);
    }
    /* Bounded Stage 1 report -> real Stage 2 resource/player construction.
       This deliberately injects report entry; it does not claim boss victory. */
    for(cycle=0;cycle<9;++cycle) {
        unsigned seen_parent=0,seen_child=0,seen_flyer=0,seen_shot=0;
        a->level=cycle%3;a->music=1;blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME);
        a->state=BLAZE_STAGE_CLEAR;a->tick=128;a->world.enemies_created=1;a->world.player.words[0x94/4]=1;
        CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_LOADING && a->pending_stage==1);
        while(a->state==BLAZE_LOADING)CHECK(blaze_app_tick(a,0));
        CHECK(a->state==BLAZE_START && a->level_data.stage==1 && a->world.tick==0);
        for(i=0;i<220;++i) {
            unsigned input=(i%4==0?BLAZE_CONFIRM:0)|(i%80<20?BLAZE_UP:i%80>=40 && i%80<60?BLAZE_DOWN:0);
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED);
            if(a->state==BLAZE_GAME_OVER)break; /* Test death, do not auto-confirm continue with held fire. */
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {
                unsigned type=a->world.actors[j][0x4c/4];
                seen_parent|=type==12;seen_child|=type==21;seen_flyer|=type==19;seen_shot|=type==3 || type==6;
            }
        }
        CHECK(seen_parent && seen_child && seen_shot && (cycle%3!=2 || seen_flyer));
        printf("Stage2 cycle=%d camera=%d enemies=%u health=%u state=%d\n",cycle,a->world.camera_x,a->world.enemies_created,a->world.player.words[0x2ad0/4],a->state);
        /* Contact deliberately sets scroll_speed=-2 (original damage path).
           Repeated hits can push the camera back behind its entry coordinate;
           the observed carrier/child and shots above establish reachability. */
        CHECK(a->world.tick>32 && a->world.enemies_created);
        if(a->state==BLAZE_GAME_OVER)CHECK(a->world.dying && !a->world.player.words[0x2ad0/4]);
        else CHECK(a->state==BLAZE_GAME && a->world.player.words[0x2ad0/4]>0);
        CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_MAIN));
        CHECK(a->state==BLAZE_TITLE && !a->pause_flags && !a->world.active && !a->level_data.tiles.bytes);
        CHECK(!a->world.actor_draw_count && !a->world.effect_draw_count && !a->world.mount_draw.sprite);
    }
    /* Bounded FERIA encounter: real warning/body spawns, then either live
       abort or injected lethal boss damage -> report -> Stage 3 resources.
       Camera and lethal damage injection are not natural-victory evidence. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned parent=256,seen_warning=0,seen_body=0,seen_blade=0,seen_weak=0;
        uint32_t warning_x=UINT32_MAX;int camera;
        a->level=cycle%3;a->music=1;blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME);
        a->state=BLAZE_STAGE_CLEAR;a->tick=128;a->world.enemies_created=1;a->world.player.words[0x94/4]=1;
        CHECK(blaze_app_tick(a,0));while(a->state==BLAZE_LOADING)CHECK(blaze_app_tick(a,0));
        CHECK(a->state==BLAZE_START && a->level_data.stage==1 && !a->world.tick);
        for(i=0;i<128;++i)if(a->level_data.spawns[i].flags && a->level_data.spawns[i].parameter==13)warning_x=a->level_data.spawns[i].x;
        CHECK(warning_x!=UINT32_MAX);camera=(int)warning_x-340;
        a->world.camera_x=camera;a->world.player.words[5]=(uint32_t)camera+80;a->world.player.words[7]=(uint32_t)camera+112;
        a->world.player.words[6]=0;a->world.player.words[8]=32;
        /* Isolate this encounter before its first world tick. Wave behavior
           remains covered separately; do not let unrelated hits stall entry. */
        for(i=0;i<128;++i)if(a->level_data.spawns[i].parameter!=4 && a->level_data.spawns[i].parameter!=13)a->level_data.spawns[i].flags=0;
        for(i=0;i<220;++i) {
            CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_START || a->state==BLAZE_GAME);
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {
                unsigned type=a->world.actors[j][0x4c/4];
                if(type==4)parent=(unsigned)j;
                seen_warning|=type==13;seen_body|=type==7;seen_blade|=type==9;seen_weak|=type==38;
            }
            if(parent<256 && a->world.actors[parent][0x54/4]>32)break;
        }
        printf("FERIA cycle=%d parent=%u warning=%u body=%u blade=%u weak=%u phase=%u camera=%d\n",cycle,parent,seen_warning,seen_body,seen_blade,seen_weak,a->world.encounter_phase,a->world.camera_x);
        CHECK(parent<256 && seen_warning && seen_body && seen_blade && seen_weak && a->world.encounter_phase==18);
        if(cycle<3) {
            CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_MAIN));
        } else {
            a->world.actors[parent][0x50/4]=UINT32_MAX;
            for(i=0;i<120 && a->state==BLAZE_GAME;++i)CHECK(blaze_app_tick(a,0));
            CHECK(a->state==BLAZE_STAGE_CLEAR && a->world.requested_screen==4);
            while(a->tick<65)CHECK(blaze_app_tick(a,0));
            CHECK(blaze_app_tick(a,BLAZE_CONFIRM));CHECK(a->state==BLAZE_LOADING && a->pending_stage==2);
            while(a->state==BLAZE_LOADING)CHECK(blaze_app_tick(a,0));
            CHECK(a->state==BLAZE_START && a->level_data.stage==2 && !a->world.tick);
            for(i=0;i<256;++i)CHECK(!a->world.actors[i][4]);
            CHECK(blaze_app_tick(a,BLAZE_ABORT));
        }
        CHECK(a->state==BLAZE_TITLE && !a->pause_flags && !a->world.active && !a->level_data.objects.bytes);
        CHECK(!a->world.actor_draw_count && !a->world.effect_draw_count && !a->world.mount_draw.sprite);
    }
    /* Stage 3 application integration: natural opening, then bounded actual
       carrier and later-attack spawns. No invulnerability or victory injection. */
    for(cycle=0;cycle<9;++cycle) {
        unsigned family=cycle<3?0:cycle<6?12:24,seen=0,seen_attack=0;
        uint32_t first=UINT32_MAX;
        a->level=cycle%3;a->music=1;blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));
        CHECK(report_next_fixture(a));CHECK(report_next_fixture(a));
        CHECK(a->level_data.stage==2 && !a->world.tick);
        if(cycle>=3) {
            for(i=0;i<128;++i)if(a->level_data.spawns[i].flags && a->level_data.spawns[i].parameter==family && a->level_data.spawns[i].x<first)first=a->level_data.spawns[i].x;
            CHECK(first!=UINT32_MAX);a->world.camera_x=(int)first-340;
            a->world.player.words[5]=(uint32_t)a->world.camera_x+80;a->world.player.words[7]=(uint32_t)a->world.camera_x+112;
            a->world.player.words[6]=0;a->world.player.words[8]=32;
            for(i=0;i<128;++i)if(a->level_data.spawns[i].x<first || (family==24 && a->level_data.spawns[i].parameter!=24 && a->level_data.spawns[i].parameter!=36))a->level_data.spawns[i].flags=0;
        }
        for(i=0;i<180;++i) {
            CHECK(blaze_app_tick(a,(i%4==0?BLAZE_CONFIRM:0)|(cycle<3 && i%80<20?BLAZE_UP:0)));
            CHECK(a->state!=BLAZE_BLOCKED);
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {
                unsigned type=a->world.actors[j][0x4c/4];seen|=type==family;seen_attack|=family==0?type==20:family==24?(type==10 || type==33):0;
            }
            for(j=0;j<8;++j)if(a->world.player.words[(0x2ddc+j*0xd4+0x10)/4])seen_attack=1;
            if(a->state==BLAZE_GAME_OVER)break;
        }
        printf("Stage3 wave cycle=%d family=%u seen=%u attack=%u health=%u state=%d\n",cycle,family,seen,seen_attack,a->world.player.words[0x2ad0/4],a->state);
        CHECK(seen && (family==0 || seen_attack));
        CHECK(a->state==BLAZE_GAME || a->state==BLAZE_GAME_OVER);
        CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_MAIN));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.objects.bytes);
        for(i=0;i<8;++i)CHECK(!a->world.player.words[(0x2ddc+i*0xd4+0x10)/4]);
    }
    /* Isolated SESIRU ownership routes. Advance the long flyby to its final
       phase and place the body at the encounter camera; the complete flyby is
       compared independently against original code. Actual warning/data spawns,
       attacks, damage/report, Stage 4 loading and teardown remain connected. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned parent=256,seen_warning=0,seen_attack=0,attack_phase=cycle%3==0?5:cycle%3==1?3:6;
        uint32_t warning_x=UINT32_MAX;
        a->level=cycle%3;a->music=1;blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));
        CHECK(report_next_fixture(a));CHECK(report_next_fixture(a));
        for(i=0;i<128;++i)if(a->level_data.spawns[i].flags && a->level_data.spawns[i].parameter==13)warning_x=a->level_data.spawns[i].x;
        CHECK(warning_x!=UINT32_MAX);a->world.camera_x=(int)warning_x-340;
        a->world.player.words[5]=(uint32_t)a->world.camera_x+80;a->world.player.words[7]=(uint32_t)a->world.camera_x+112;
        a->world.player.words[6]=0;a->world.player.words[8]=32;
        for(i=0;i<128;++i)if(a->level_data.spawns[i].parameter!=4 && a->level_data.spawns[i].parameter!=13)a->level_data.spawns[i].flags=0;
        CHECK(blaze_app_tick(a,0));
        for(i=0;i<256;++i)if(a->world.actors[i][4] && a->world.actors[i][0x4c/4]==4)parent=(unsigned)i;
        CHECK(parent<256);
        {uint32_t *body=a->world.actors[parent];uint32_t width=body[7]-body[5],height=body[8]-body[6];
            body[5]=(uint32_t)a->world.camera_x+200;body[6]=160;body[7]=body[5]+width;body[8]=body[6]+height;body[0x54/4]=1330;}
        CHECK(blaze_app_tick(a,0));CHECK(a->world.actors[parent][0x44/4]==0xb04);
        a->world.actors[parent][0x58/4]=attack_phase;a->world.actors[parent][0x54/4]=0;
        for(i=0;i<70;++i) {
            CHECK(blaze_app_tick(a,attack_phase==6 && i>=32?BLAZE_DOWN:0));
            if(a->state!=BLAZE_START && a->state!=BLAZE_GAME)fprintf(stderr,"SESIRU cycle=%d tick=%zu phase=%u state=%u health=%u error=%s\n",cycle,i,attack_phase,a->state,a->world.player.words[0x2ad0/4],a->error);
            CHECK(a->state==BLAZE_START || a->state==BLAZE_GAME);
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {unsigned type=a->world.actors[j][0x4c/4];seen_warning|=type==13;seen_attack|=attack_phase==3?type==29:attack_phase==6?type==18:0;}
            for(j=0;j<8;++j)if(a->world.player.words[(0x2ddc+j*0xd4+0x10)/4])seen_attack=1;
        }
        CHECK(seen_warning && seen_attack && a->world.encounter_phase==18 && a->state==BLAZE_GAME);
        if(cycle<3) {CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_MAIN));}
        else {
            a->world.actors[parent][0x50/4]=UINT32_MAX;
            for(i=0;i<120 && a->state==BLAZE_GAME;++i)CHECK(blaze_app_tick(a,attack_phase==6?BLAZE_DOWN:0));
            CHECK(a->state==BLAZE_STAGE_CLEAR && a->world.requested_screen==4);
            while(a->tick<65)CHECK(blaze_app_tick(a,0));
            CHECK(blaze_app_tick(a,BLAZE_CONFIRM));CHECK(a->state==BLAZE_LOADING && a->pending_stage==3);
            while(a->state==BLAZE_LOADING)CHECK(blaze_app_tick(a,0));
            CHECK(a->state==BLAZE_START && a->level_data.stage==3 && !a->world.tick);
            for(i=0;i<256;++i)CHECK(!a->world.actors[i][4]);
            CHECK(blaze_app_tick(a,BLAZE_ABORT));
        }
        CHECK(a->state==BLAZE_TITLE && !a->pause_flags && !a->world.active && !a->level_data.tiles.bytes);
        CHECK(!a->world.actor_draw_count && !a->world.effect_draw_count && !a->world.mount_draw.sprite);
    }
    /* The dormant pool can execute through the actual world pipeline too.
       Activation is fixture-only; no new action/weapon has been invented. */
    {
        CHECK(blaze_level_open(&a->level_data,argv[1],0,1,error,sizeof(error)));
        CHECK(blaze_world_begin(&a->world,&a->level_data,&a->font,&a->loadout,&a->random,a->trig));
        for(i=0;i<5;++i) {
            uint32_t *shot=a->world.player.words+(0xec+i*0x34)/4;
            shot[4]=1;shot[11]=(uint32_t)i*32;shot[5]=100;shot[6]=120;
            shot[7]=100+blaze_level_sprite(&a->level_data,shot[0])->width;
            shot[8]=120+blaze_level_sprite(&a->level_data,shot[0])->visible_height;
        }
        for(i=0;i<50;++i)CHECK(blaze_world_step(&a->world,a->trig,0,error,sizeof(error)));
        for(i=0;i<5;++i)CHECK(!a->world.player.words[(0xec+i*0x34+0x10)/4]);
        blaze_world_end(&a->world);blaze_level_close(&a->level_data);
        CHECK(!a->world.effect_draw_count);
    }
    /* Force the original inclusive endpoint: 11 is outside the weapon table
       and aliases starfield X in the PE. The native bounds fix rerolls only
       this invalid value, then uses the actual pickup/equipment lifecycle. */
    {
        uint32_t rect[]={220,96,252,128};unsigned slot=256;
        CHECK(blaze_level_open(&a->level_data,argv[1],0,1,error,sizeof(error)));
        a->loadout.mode=0;a->loadout.ship=0;a->loadout.weapon=1;
        for(i=1;i<11;++i)a->loadout.weapons[i].enabled=i==5;
        CHECK(blaze_world_begin(&a->world,&a->level_data,&a->font,&a->loadout,&a->random,a->trig));
        CHECK(blaze_actor_spawn(&a->world,rect,11,3,0));
        for(i=0;i<256;++i)if(a->world.actors[i][4])slot=(unsigned)i;
        CHECK(slot<256);a->random.state=0xf01bf641;
        CHECK(blaze_actor_update(&a->world,a->world.actors[slot],1,error,sizeof(error)));
        CHECK(a->random.state==0x3c299ec3 && a->world.actors[slot][11]==5 && a->world.actors[slot][4]);
        memcpy(a->world.player.words+5,a->world.actors[slot]+5,4*sizeof(uint32_t));
        CHECK(blaze_actor_update(&a->world,a->world.actors[slot],1,error,sizeof(error)));
        CHECK(a->world.player.words[0xb0/4]==5 && !a->world.actors[slot][4]);
        for(i=0;i<11;++i)a->loadout.weapons[i].enabled=i<3;
        blaze_world_end(&a->world);blaze_level_close(&a->level_data);
    }
    /* Later shipped wave records. Only the approach camera and already-passed
       spawns are bounded; enemies, attacks, damage and teardown run normally. */
    for(cycle=0;cycle<17;++cycle) {
        static const unsigned families[]={1,2,12,22,40,42};
        unsigned family=families[cycle/3],stage=family==42?4:3,seen=0;
        uint32_t first=UINT32_MAX;
        /* Easy's final stage contains only warning/boss, no type-42 records. */
        a->level=cycle<15?cycle%3:cycle-14;a->music=1;blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));
        for(j=0;j<stage;++j)CHECK(report_next_fixture(a));
        for(i=0;i<128;++i)if(a->level_data.spawns[i].flags && a->level_data.spawns[i].parameter==family && a->level_data.spawns[i].x<first)first=a->level_data.spawns[i].x;
        CHECK(first!=UINT32_MAX);a->world.camera_x=(int)first-340;
        a->world.player.words[5]=(uint32_t)a->world.camera_x+80;a->world.player.words[7]=(uint32_t)a->world.camera_x+112;
        a->world.player.words[6]=0;a->world.player.words[8]=32;
        for(i=0;i<128;++i)if(a->level_data.spawns[i].x<first || a->level_data.spawns[i].parameter!=family)a->level_data.spawns[i].flags=0;
        for(i=0;i<120;++i) {
            CHECK(blaze_app_tick(a,i%4==0?BLAZE_CONFIRM:0));CHECK(a->state!=BLAZE_BLOCKED);
            for(j=0;j<256;++j)if(a->world.actors[j][4] && a->world.actors[j][0x4c/4]==family)seen=1;
            if(a->state==BLAZE_GAME_OVER)break;
        }
        CHECK(seen);CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_MAIN));
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.tiles.bytes && !a->world.actor_draw_count);
    }
    /* ISABEL/RAIYA: real data construction, bounded approach and selected attack
       phase; alternate active-combat abort and lethal damage -> report -> next
       level/ending. These injected fixtures do not prove natural victory. */
    for(cycle=0;cycle<12;++cycle) {
        unsigned stage=cycle<6?3:4,parent=256,seen=0,phase=stage==3?4:5;
        uint32_t boss_x=UINT32_MAX;
        a->level=cycle%3;a->music=1;blaze_random_begin(&a->random,1);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));
        for(j=0;j<stage;++j)CHECK(report_next_fixture(a));
        for(i=0;i<128;++i)if(a->level_data.spawns[i].flags && a->level_data.spawns[i].parameter==4)boss_x=a->level_data.spawns[i].x;
        CHECK(boss_x!=UINT32_MAX);a->world.camera_x=(int)boss_x-319;
        a->world.player.words[5]=(uint32_t)a->world.camera_x+80;a->world.player.words[7]=(uint32_t)a->world.camera_x+112;
        a->world.player.words[6]=0;a->world.player.words[8]=32;
        for(i=0;i<128;++i)if(a->level_data.spawns[i].parameter!=4)a->level_data.spawns[i].flags=0;
        CHECK(blaze_app_tick(a,0));
        for(i=0;i<256;++i)if(a->world.actors[i][4] && a->world.actors[i][0x4c/4]==4)parent=(unsigned)i;
        CHECK(parent<256);a->world.actors[parent][0x58/4]=phase;a->world.actors[parent][0x54/4]=0;
        for(i=0;i<40;++i) {
            CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_START || a->state==BLAZE_GAME);
            for(j=0;j<256;++j)if(a->world.actors[j][4] && a->world.actors[j][0x4c/4]==(stage==3?37u:28u))seen=1;
        }
        CHECK(seen);
        if(cycle%6<3) {CHECK(press(a,BLAZE_PAUSE));CHECK(press(a,BLAZE_MAIN));}
        else {
            a->world.actors[parent][0x50/4]=UINT32_MAX;
            for(i=0;i<120 && a->state==BLAZE_GAME;++i)CHECK(blaze_app_tick(a,0));
            CHECK(a->state==BLAZE_STAGE_CLEAR);
            while(a->state==BLAZE_STAGE_CLEAR)CHECK(blaze_app_tick(a,0));
            if(stage==3) {
                while(a->state==BLAZE_LOADING)CHECK(blaze_app_tick(a,0));
                CHECK(a->state==BLAZE_START && a->level_data.stage==4);
            } else {
                unsigned elapsed=0;
                CHECK(a->state==BLAZE_ENDING);
                while(a->ending_card<9 || a->tick<260) {CHECK(elapsed++<900);CHECK(blaze_app_tick(a,0));}
                CHECK(a->screen.count==1);CHECK(press(a,BLAZE_CONFIRM));CHECK(a->state==BLAZE_INTRO);
            }
            CHECK(press(a,BLAZE_MAIN));
        }
        CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.tiles.bytes && !a->world.effect_draw_count);
    }
    puts("Stages 4/5 shipped waves, boss attacks, active abort, death/report, next-level/credits and repeated teardown passed");
    for (cycle=0;cycle<15;++cycle) {
        CHECK(blaze_level_open(&a->level_data,argv[1],(unsigned)cycle%5,(unsigned)cycle%3,error,sizeof(error)));
        CHECK(a->level_data.objects.count==304);
        CHECK(a->level_data.width[1]==853 && a->level_data.height[1]==11);
        CHECK(!blaze_level_open(&a->level_data,argv[1],5,0,error,sizeof(error)));
        CHECK(a->level_data.map[0]); /* Failed load keeps current world owned. */
    }
    for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
    CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
    for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));a->world.player.words[0x2ad0/4]=0;
    for(i=0;i<34;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME_OVER && a->game_over_image.bytes);
    blaze_app_close(a);CHECK(!a->game_over_image.bytes && !a->world.active);
    blaze_app_close(a);
    /* Fixed MIDI choice keeps this gameplay regression independent of music rolls.
       Full seed-1 prefix, without injected world state: movement changes the camera,
       tap fire fights the actual waves, and type 12 creates its own child. */
    CHECK(blaze_app_open(a,argv[1],argv[2]));a->music=1;blaze_random_begin(&a->random,1);
    {
        unsigned seen_parent=0,seen_child=0,seen_boss=0,seen_warning=0;
        for(i=0;i<3340;++i) {
            unsigned input=(i==40 || i==42 || i==44 || i==46 || (i>=168 && i%4==0))?BLAZE_CONFIRM:0;
            if(i>=168) {unsigned phase=(unsigned)(i-168)%160;if(phase<40)input|=BLAZE_DOWN;else if(phase>=80 && phase<120)input|=BLAZE_UP;}
            CHECK(blaze_app_tick(a,input));CHECK(a->state!=BLAZE_BLOCKED && a->state!=BLAZE_GAME_OVER);
            for(j=0;j<256;++j)if(a->world.actors[j][4]) {
                seen_parent|=a->world.actors[j][0x4c/4]==12;seen_child|=a->world.actors[j][0x4c/4]==21;
                seen_boss|=a->world.actors[j][0x4c/4]==4;seen_warning|=a->world.actors[j][0x4c/4]==13;
            }
        }
        CHECK(seen_parent && seen_child && seen_boss && seen_warning && a->state==BLAZE_GAME && a->world.camera_x>17000 && a->world.encounter_phase==18);
        CHECK(a->world.player.words[0x2ad0/4]>0 && a->world.player.words[0x58/4]>0);
        printf("Native 3340-frame movement/combat/boss prefix: camera=%d health=%u score=%u rng=%08x\n",
            a->world.camera_x,a->world.player.words[0x2ad0/4],a->world.player.words[0x58/4],a->random.state);
        CHECK(blaze_app_tick(a,BLAZE_ABORT));CHECK(a->state==BLAZE_TITLE && !a->world.active && !a->level_data.tiles.bytes);
        for(i=0;i<5;++i)CHECK(blaze_app_tick(a,0));
        CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));CHECK(press(a,BLAZE_CONFIRM));
        for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_GAME);
        for(i=0;i<256;++i)CHECK(!a->world.actors[i][4]);
    }
    blaze_app_close(a);free(a);
    puts("Native menus, combat/score, contact damage, RNG persistence, repeated combat abort/reload and unsupported routes passed");
    return 0;
fail:
    if (a) { fprintf(stderr,"%s\n",a->error); blaze_app_close(a); free(a); }
    blaze_archive_free(&archive); return 1;
}
