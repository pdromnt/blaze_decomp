#include "blaze.h"
#include <stdio.h>
#include <string.h>

static void typing(BlazeApp *a,const char *str,int y) {
    size_t i,n=strlen(str),visible=a->clear_characters++/2;int width=a->font.sprites[0].width,x=160-(int)n*width/2;
    if(n>visible)n=visible;
    for(i=0;i<n;++i)if(str[i]>=32 && str[i]<=90)blaze_draw(&a->frame,&a->font,(unsigned)str[i]-32,x+(int)i*width,y,NULL);
}
/* 0x403f20: retain the world through the opening flash, then report the
   actual registry miss count and continue with the current player selection.
   The platform persists awarded unlocks before advancing to another stage. */
int blaze_stage_clear_tick(BlazeApp *a,unsigned input) {
    BlazeWorld *w=&a->world;uint32_t *p=w->player.words;unsigned actions=0,t=a->tick,unlock=a->level_data.stage+3;char line[128];
    p[0xa4/4]=input;if(input==p[0x60/4])p[0xa4/4]=0;else {p[0x60/4]=input;actions=input;}
    if(t<15) {
        if(!((t/2)&1)) {
            if(!blaze_world_step(w,a->trig,input,a->error,sizeof(a->error))) {a->state=BLAZE_BLOCKED;return 1;}
            blaze_world_draw(w,&a->frame,a->rop);
        } else memset(a->frame.pixels,255,sizeof(a->frame.pixels));
    } else {
        w->camera_x=w->camera_y=0;memset(a->frame.pixels,0,sizeof(a->frame.pixels));
        blaze_text(&a->frame,&a->font,"STAGE CLEAR",0,80,4,a->rop);
    }
    if(t==56 && !p[0x94/4])p[0x58/4]+=1000;
    if(t==32) {if(!w->enemies_created)w->enemies_created=1;a->clear_characters=0;}
    if(t>31) {
        int percent=blaze_signed(((uint32_t)w->enemies_created-p[0x94/4])*100u)/(int)w->enemies_created;
        snprintf(line,sizeof(line),"ENEMY DESTROY %d%%",percent);typing(a,line,112);
        if(a->loadout.mode==4 && !p[0x94/4] && (t&1))blaze_text(&a->frame,&a->font,"RATE BONUS 1000",0,128,4,a->rop);
        snprintf(line,sizeof(line),"MISS %d",blaze_signed(p[0x94/4]));typing(a,line,144);
        if(a->loadout.mode==4) {snprintf(line,sizeof(line),"SCORE:%d",blaze_signed(p[0x58/4]));typing(a,line,176);}
        if(!p[0x94/4] && !a->loadout.weapons[unlock].enabled && (t&1)) {
            snprintf(line,sizeof(line),"%s GET",a->loadout.weapons[unlock].name);blaze_text(&a->frame,&a->font,line,0,224,2,a->rop);
        }
    }
    ++a->tick;
    if((t>64 && (actions&BLAZE_CONFIRM)) || a->tick>128) {
        if(!p[0x94/4] && !a->loadout.weapons[unlock].enabled) {
            a->loadout.weapons[unlock].enabled=1;if(!blaze_progress_save(a))return 0;
        }
        a->pending_stage=a->level_data.stage+1;a->tick=0;
        if(a->pending_stage<a->max_stage) {a->state=BLAZE_LOADING;a->restart_pending=1;}
        else a->state=BLAZE_ENDING;
    }
    return 1;
}
