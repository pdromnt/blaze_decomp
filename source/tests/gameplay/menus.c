#include "blaze.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"menus line %d: %s\n",__LINE__,#x);goto done;}} while(0)
typedef struct {unsigned loads,plays,stops,sounds,displays,saves;int ended,mode,level,music,fail;} Services;
static void load(void *p,unsigned slot,const char *file,unsigned track,int cd) {
    Services *s=p;(void)slot;(void)file;(void)track;(void)cd;++s->loads;
}
static void play(void *p,unsigned slot,int loop) {Services *s=p;(void)slot;(void)loop;++s->plays;s->ended=0;}
static void stop(void *p) {Services *s=p;++s->stops;s->ended=0;}
static void sound(void *p,unsigned id) {Services *s=p;(void)id;++s->sounds;}
static int ended(void *p) {return ((Services *)p)->ended;}
static int display(void *p,int mode) {Services *s=p;++s->displays;s->mode=mode;return !s->fail;}
static int save(void *p,int level,int mode,int music) {
    Services *s=p;++s->saves;s->level=level;s->mode=mode;s->music=music;return !s->fail;
}
static int press(BlazeApp *a,unsigned input) {return blaze_app_tick(a,0) && blaze_app_tick(a,input);}
static int settle(BlazeApp *a) {while(a->tick<25)if(!blaze_app_tick(a,0))return 0;return 1;}
int main(int argc,char **argv) {
    BlazeApp *a=calloc(1,sizeof(*a));Services s={0};unsigned i,cycle;int result=1;
    BlazeMedia media={&s,sound,load,play,stop,NULL,ended};
    CHECK(argc==3 && a && blaze_app_open(a,argv[1],argv[2],NULL));
    a->settings_context=&s;a->apply_display=display;a->save_options=save;
    blaze_app_set_media(a,&media);a->music=1;
    for(i=0;i<40;++i)CHECK(blaze_app_tick(a,0));
    CHECK(press(a,BLAZE_DOWN) && press(a,BLAZE_DOWN) && press(a,BLAZE_CONFIRM));
    CHECK(a->state==BLAZE_OPTIONS && a->menu_image.bytes);
    {unsigned loads=s.loads;
        for(i=0;i<16;++i) {CHECK(blaze_app_tick(a,0));CHECK(s.loads==loads);}
        CHECK(a->tick==16 && !a->menu_image.bytes);
        for(i=0;i<76800;++i)CHECK(!a->frame.pixels[i]);
        CHECK(blaze_app_tick(a,0) && s.loads==loads+1 && a->music_group[0]==3);
    }
    CHECK(settle(a));
    {int old=a->level;CHECK(press(a,BLAZE_LEFT));CHECK(a->level==old && a->level_pending==0);
        CHECK(press(a,BLAZE_CONFIRM));CHECK(a->level==old && !s.saves);
        CHECK(press(a,BLAZE_CANCEL) && a->state==BLAZE_TITLE && a->level==old);
    }
    CHECK(settle(a) && press(a,BLAZE_CONFIRM) && settle(a));
    CHECK(a->level_pending==a->level);
    CHECK(press(a,BLAZE_DOWN) && press(a,BLAZE_LEFT));
    CHECK(!s.displays && a->display==1 && a->display_pending==0);
    CHECK(press(a,BLAZE_CONFIRM));CHECK(s.displays==1 && a->display==0 && s.mode==0);
    CHECK(press(a,BLAZE_DOWN) && press(a,BLAZE_RIGHT));
    CHECK(a->music==1 && a->music_pending==2);
    CHECK(press(a,BLAZE_DOWN) && press(a,BLAZE_CONFIRM));
    CHECK(a->state==BLAZE_RANKING && a->ranking_return && settle(a));
    CHECK(a->ranking_scores[0]==6000 && a->ranking_scores[4]==2000 && a->music_group[0]==4);
    CHECK(a->ranking_modes[0]==4 && a->ranking_modes[4]==0);
    CHECK(press(a,BLAZE_CANCEL) && a->state==BLAZE_OPTIONS && settle(a));
    CHECK(a->music_pending==1 && a->display_pending==0); /* Re-entry discards uncommitted values. */
    CHECK(press(a,BLAZE_UP) && press(a,BLAZE_RIGHT));
    CHECK(press(a,BLAZE_DOWN) && press(a,BLAZE_DOWN) && press(a,BLAZE_CONFIRM));
    CHECK(a->state==BLAZE_TITLE && a->music==2 && s.saves==2 && s.music==2 && s.mode==0);
    /* Repeated owner replacement, both ranking return routes and EOF timing. */
    for(cycle=0;cycle<20;++cycle) {
        CHECK(settle(a));s.ended=1;CHECK(blaze_app_tick(a,0));
        CHECK(a->state==BLAZE_RANKING && !a->ranking_return && settle(a));
        s.ended=1;CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_INTRO && !a->menu_image.bytes);
        for(i=0;i<32;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_TITLE);
    }
    /* Entire credit sequence, strict score insertion, session unlock, real end
       archive, confirm gate, music EOF and app-owned resource release. */
    for(cycle=0;cycle<6;++cycle) {
        unsigned elapsed=0,mode=cycle&1?4:0;
        a->state=BLAZE_ENDING;a->tick=0;a->previous_input=0;a->loadout.mode=mode;
        for(i=0;i<5;++i) {a->ranking_scores[i]=6000-(int)i*1000;a->ranking_modes[i]=(int)i;}
        for(i=7;i<11;++i)a->loadout.weapons[i].enabled=i<8;
        a->world.player.words[0x58/4]=cycle<2?7000:cycle<4?6000:1000;
        CHECK(blaze_app_tick(a,BLAZE_CONFIRM));CHECK(a->state==BLAZE_ENDING && a->music_group[0]==5);
        while(a->ending_card<9 || a->tick<260) {
            CHECK(elapsed++<900);CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_ENDING);
            if(a->tick==20) {
                int rank=mode==4?(cycle<2?0:cycle<4?1:-1):-1;
                for(i=0;i<5;++i)CHECK(a->ranking_modes[i]==((int)i==rank?4:rank>=0 && (int)i>rank?(int)i-1:(int)i));
                if(mode==4)CHECK(a->ending_rank==rank);
            }
            if(a->tick==66 && a->ending_card>=2 && !(a->ending_card&1)) {
                static const unsigned bodies[]={0x2b4,0x2cc,0x2ae,0x29c};
                const BlazeSprite *sp=&a->screen.sprites[bodies[a->ending_card/2-1]-0x170];
                unsigned x,y,checked=0;
                /* Regression: palette conversion is a 256-byte source lookup,
                   never the 65536-byte source/destination ROP operation. Check
                   actual pixels against the independent decoded image. */
                for(y=0;y<sp->visible_height;++y)for(x=0;x<sp->width;++x) {
                    int dx=(int)x-sp->origin_x,dy=(int)y-sp->origin_y;
                    uint8_t pixel=a->screen.expanded[sp->expanded+(size_t)y*sp->width+x];
                    if(pixel==(uint8_t)sp->transparent || dx<0 || dx>=320 || dy<0 || dy>=240)continue;
                    CHECK(a->frame.pixels[dy*320+dx]==a->game_over_colors[pixel]);++checked;
                }
                CHECK(checked>0);
            }
        }
        CHECK(a->screen.count==1 && !a->loadout.weapons[8].enabled);
        CHECK(press(a,BLAZE_CONFIRM));CHECK(a->loadout.weapons[8].enabled && !a->loadout.weapons[9].enabled);
        CHECK(!a->world.active && !a->level_data.tiles.bytes);
        if(mode==4) {CHECK(a->state==BLAZE_RANKING && !a->ranking_return && settle(a));CHECK(press(a,BLAZE_CONFIRM));}
        CHECK(a->state==BLAZE_INTRO);
        for(i=0;i<32;++i)CHECK(blaze_app_tick(a,0));CHECK(a->state==BLAZE_TITLE);
    }
    a->state=BLAZE_ENDING;a->tick=0;a->loadout.mode=0;a->loadout.weapons[9].enabled=0;
    CHECK(blaze_app_tick(a,0));s.ended=1;CHECK(blaze_app_tick(a,0));
    CHECK(a->state==BLAZE_INTRO && a->loadout.weapons[9].enabled);
    for(i=0;i<32;++i)CHECK(blaze_app_tick(a,0));
    CHECK(settle(a) && press(a,BLAZE_CONFIRM) && settle(a));
    CHECK(a->state==BLAZE_OPTIONS);
    for(i=0;i<4;++i)CHECK(press(a,BLAZE_UP));
    CHECK(press(a,BLAZE_DOWN) && a->display_pending==0);
    CHECK(press(a,BLAZE_RIGHT) && a->display_pending==1);
    CHECK(press(a,BLAZE_RIGHT) && a->display_pending==5);
    CHECK(press(a,BLAZE_CONFIRM) && a->display==5 && s.mode==5);
    CHECK(press(a,BLAZE_RIGHT) && a->display_pending==2);
    CHECK(press(a,BLAZE_LEFT) && a->display_pending==5);
    CHECK(press(a,BLAZE_LEFT) && a->display_pending==1);
    CHECK(press(a,BLAZE_LEFT) && a->display_pending==0);
    CHECK(press(a,BLAZE_CONFIRM) && a->display==0 && s.mode==0);
    for(i=0;i<3;++i)CHECK(press(a,BLAZE_DOWN));
    s.fail=1;CHECK(!press(a,BLAZE_CONFIRM));CHECK(strstr(a->error,"save user options"));
    result=0;puts("Menu transitions, delayed cues, option transactions, ranking/EOF and repeated teardown passed");
done:
    if(a)blaze_app_close(a);free(a);return result;
}
