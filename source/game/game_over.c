#include "blaze.h"
#include <stdio.h>
#include <string.h>

/* 0x404a70. Snapshot/palette ownership is native; world updates and draws use
   the same game implementation as normal play, with the original pulse order. */
static void caption(BlazeApp *a,const char *text,int y,const uint8_t *colors) {
    size_t i;int width=a->font.sprites[0].width;
    int x=160-(int)strlen(text)*width/2;
    for(i=0;text[i];++i)if(text[i]>=32 && text[i]<=90)
        blaze_draw_scaled(&a->frame,&a->font,(unsigned)text[i]-32,x+(int)i*width,y,256,256,NULL,colors);
}
int blaze_game_over_tick(BlazeApp *a,unsigned input) {
    BlazeWorld *w=&a->world;uint32_t *p=w->player.words;unsigned actions=0,t=a->tick;
    p[0xa4/4]=input;
    if(input==p[0x60/4])p[0xa4/4]=0;
    else {p[0x60/4]=input;actions=input;}
    if(t<32 && ((t/2)&1)) {
        int32_t x=w->camera_x,y=w->camera_y;unsigned i;
        w->camera_x+=(int)(blaze_random_next(w->random)*16/32767)-8;
        w->camera_y+=(int)(blaze_random_next(w->random)*16/32767)-8;
        if(!blaze_world_step(w,a->trig,input,a->error,sizeof(a->error))) {
            w->camera_x=x;w->camera_y=y;a->state=BLAZE_BLOCKED;return 1;
        }
        blaze_world_draw(w,&a->frame,a->rop);w->camera_x=x;w->camera_y=y;
        blaze_sound(w,BLAZE_KIL);
        for(i=0;i<3;++i) {
            const BlazeSprite *s=blaze_level_sprite(w->level,0x81);
            int dy=(int)(blaze_random_next(w->random)*32/32767)-16;
            int dx=(int)(blaze_random_next(w->random)*64/32767)-16;
            unsigned frame=blaze_random_next(w->random)*4/32767;
            int sx=blaze_signed(p[5]+(uint32_t)dx)+(blaze_signed(p[7]-p[5])/2-s->width/2)-w->camera_x;
            int sy=blaze_signed(p[6]+(uint32_t)dy)+(blaze_signed(p[8]-p[6])/2-s->visible_height/2)-w->camera_y;
            blaze_draw(&a->frame,&w->level->objects,0x81-0x40+frame,sx,sy,NULL);
        }
    }
    if(t==32) {
        blaze_music_load(a,6,0);blaze_music_play(a,0,1);
        unsigned i;w->camera_x=w->camera_y=0;
        if(!blaze_frame_capture(&a->game_over_image,&a->frame)) {
            snprintf(a->error,sizeof(a->error),"Cannot capture game-over image");return 0;
        }
        /* The original conversion uses font-source colors, including for the
           captured image. Preserve this cross-palette mapping deliberately. */
        blaze_color_map(a->game_over_colors,a->frame.palette,a->font.bytes+12,0xff0000,50);
        for(i=0;i<6;++i)blaze_color_map(a->game_over_text_colors[i],a->frame.palette,a->font.bytes+12,0x20dc,i*15);
    }
    if(!(t&127))a->game_over_caption=0;
    if(t>31) {
        int x=0,y=0,scale=128,color=5-(int)(a->game_over_caption/2);
        if(t<64)scale=(96-(int)t)*4;
        else if(t<96) {x=(int)t*4-256;y=(int)t*3-192;}
        else {x=128;y=96;}
        memset(a->frame.pixels,0,sizeof(a->frame.pixels));
        blaze_draw_scaled(&a->frame,&a->game_over_image,0,x,y,scale,scale,NULL,a->game_over_colors);
        if(color<0)color=0;
        caption(a,"< GAME OVER >",112,a->game_over_text_colors[color]);++a->game_over_caption;
    }
    if(t>47 && (t&15)<12)blaze_text(&a->frame,&a->font,"CONTINUE YES-Z NO-X",0,144,4,a->rop);
    if(t>48 && (actions&BLAZE_CONFIRM)) {
        blaze_music_stop(a);blaze_sound(w,BLAZE_PUU);
        p[0x58/4]=0;if(a->loadout.mode==4)p[0x5c/4]=0;
        a->state=BLAZE_LOADING;a->tick=0;a->restart_pending=1;
        a->pending_stage=a->level_data.stage;
        blaze_archive_free(&a->game_over_image);
    }
    ++a->tick;
    if(a->tick>1280 || (a->tick>48 && (actions&BLAZE_CANCEL))) {
        blaze_music_stop(a);blaze_sound(w,BLAZE_PUU);
        a->state=BLAZE_TITLE;a->tick=0;blaze_archive_free(&a->game_over_image);
    }
    return 1;
}
