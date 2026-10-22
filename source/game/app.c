#include "blaze.h"
#include "blaze_files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

static int clamp(int value, int low, int high) {
    return value<low ? low : value>high ? high : value;
}
static int config_integer(const char *value) {
    long number=strtol(value,NULL,10);
    return number>INT_MAX ? INT_MAX : number<INT_MIN ? INT_MIN : (int)number;
}
static int key_index(const char *key, const char *prefix) {
    char *end; long index; size_t length=strlen(prefix);
    if (strncmp(key,prefix,length)) return -1;
    index=strtol(key+length,&end,10);
    return end!=key+length && !*end && index>=0 && index<=11 ? (int)index : -1;
}
/* Read-only startup configuration; writes must later target the user save dir.
   This bounded reader covers the shipped section/key syntax. */
static void config(BlazeApp *a, const char *name, int score) {
    char line[256], section[80]="";
    FILE *f;
    if (!(f=blaze_asset_open(name))) return;
    while (fgets(line,sizeof(line),f)) {
        char key[80],value[128]; int index;
        if (sscanf(line,"[%79[^]]]",section)==1) continue;
        if (sscanf(line,"%79[^=]=%127[^\r\n]",key,value)!=2) continue;
        if (!score && !strcmp(section,"Enh")) {
            if (!strcmp(key,"Time")) a->frame_ms=(unsigned)clamp(config_integer(value),1,60000);
            if (!strcmp(key,"Level")) a->level=clamp(config_integer(value),0,2);
            if (!strcmp(key,"Screen")) a->display=clamp(config_integer(value),0,4);
            if (!strcmp(key,"MidPlayer")) a->music=clamp(config_integer(value),0,5);
            if (!strcmp(key,"MaxStage")) a->max_stage=(unsigned)clamp(config_integer(value),1,5);
            index=key_index(key,"WeaponName");
            if (index>=1 && index<=11)
                snprintf(a->loadout.weapons[index-1].name,32,"%.31s",value);
            index=key_index(key,"WeaponPower");
            if(index>=1 && index<=11)a->loadout.weapons[index-1].power=config_integer(value);
            index=key_index(key,"WeaponNo");
            if(index>=1 && index<=11)a->loadout.weapons[index-1].action=config_integer(value);
            if(!strncmp(key,"WeaponX",7) || !strncmp(key,"WeaponY",7)) {
                int number,mount; char tail;
                if(sscanf(key+7,"%2d%1d%c",&number,&mount,&tail)==2 && number>=1 && number<=11 && mount>=1 && mount<=3) {
                    if(key[6]=='X')a->loadout.weapons[number-1].mount_x[mount-1]=config_integer(value);
                    else a->loadout.weapons[number-1].mount_y[mount-1]=config_integer(value);
                }
            }
        }
        if(!score && !strcmp(section,"MIDI"))blaze_music_key(a,key,value);
        if (score && !strcmp(section,"Score")) {
            index=key_index(key,"HiScore");
            if(index>=1 && index<=5)a->ranking_scores[index-1]=config_integer(value);
            index=key_index(key,"ClearMode");
            if(index>=1 && index<=5)a->ranking_modes[index-1]=config_integer(value);
            a->high_score=a->ranking_scores[0];
            index=key_index(key,"WeaponFlag");
            if (index>=0 && index<11) a->loadout.weapons[index].enabled=config_integer(value)!=0;
        }
    }
    fclose(f);
}
static int screen(BlazeApp *a, BlazeScreen state, const char *file, size_t palette) {
    char path[1200]; BlazeArchive next={0};
    if (!blaze_path(path,sizeof(path),a->root,file) ||
        !blaze_archive_read(&next,path,a->error,sizeof(a->error))) return 0;
    if (palette>=next.count) {
        blaze_archive_free(&next); snprintf(a->error,sizeof(a->error),"Missing screen palette"); return 0;
    }
    /* Build before retiring the old owner: a failed load preserves it. */
    blaze_archive_free(&a->screen); a->screen=next;
    memcpy(a->frame.palette,a->screen.bytes+12,1024);
    memset(a->frame.pixels,0,sizeof(a->frame.pixels));
    a->state=state; a->tick=0; return 1;
}
/* 0x4191c0: one app-owned immutable outgoing image. Assets are constructed
   transactionally on entry; their palette becomes visible at original tick 16. */
static int menu(BlazeApp *a,BlazeScreen state) {
    uint8_t palette[1024];
    if(!blaze_frame_capture(&a->menu_image,&a->frame)) {
        snprintf(a->error,sizeof(a->error),"Cannot capture menu transition");return 0;
    }
    memcpy(palette,a->frame.palette,1024);
    if(!screen(a,state,"dar/BLZ_SEL.DAR",12))return 0;
    memcpy(a->frame.palette,palette,1024);
    blaze_draw(&a->frame,&a->menu_image,0,0,0,NULL);
    a->level_pending=a->level;a->display_pending=a->display;a->music_pending=a->music;
    return 1;
}
int blaze_menu_transition(BlazeApp *a) {
    if(!a->tick && !blaze_frame_capture(&a->menu_image,&a->frame)) {
        snprintf(a->error,sizeof(a->error),"Cannot capture menu transition");return 0;
    }
    if(a->tick<=23)memset(a->frame.pixels,0,sizeof(a->frame.pixels));
    if(a->tick<9) {
        int scale=258-(int)a->tick*32,y=(int)a->tick*15;
        int height=240*scale/256,bottom=y+height,row,first;
        if(bottom>240)bottom=240;
        first=(y+height-bottom)*256/scale;
        /* 0x427d00 differs from the later 0x429220 scaled renderer at
           clipped enlargement: its row accumulator restarts after clipping.
           The owned capture has exactly 240 opaque 320-byte span rows. */
        for(row=0;row<bottom-y;++row) {
            int source=first+(scale<256?row*256/scale:((row+1)*256+scale-1)/scale-1);
            if(source>=0 && source<240)memcpy(a->frame.pixels+(bottom-1-row)*320,
                a->menu_image.bytes+(size_t)source*324+4,320);
        }
    }
    if(a->tick==9)blaze_archive_free(&a->menu_image);
    return 1;
}
static void sound(BlazeApp *a,unsigned id) {if(a->media.sound)a->media.sound(a->media.context,id);}
static int display(BlazeApp *a,int mode) {
    if(a->apply_display && !a->apply_display(a->settings_context,mode)) {
        snprintf(a->error,sizeof(a->error),"Cannot apply display mode %d",mode);return 0;
    }
    a->display=mode;return 1;
}
static int save_options(BlazeApp *a) {
    if(a->save_options && !a->save_options(a->settings_context,a->level,a->display,a->music)) {
        snprintf(a->error,sizeof(a->error),"Cannot save user options");return 0;
    }
    return 1;
}
int blaze_progress_save(BlazeApp *a) {
    unsigned i,flags=0;
    for(i=0;i<11;++i)if(a->loadout.weapons[i].enabled)flags|=1u<<i;
    if(a->save_progress && !a->save_progress(a->settings_context,a->ranking_scores,a->ranking_modes,flags)) {
        snprintf(a->error,sizeof(a->error),"Cannot save user progress");return 0;
    }
    return 1;
}
int blaze_app_open(BlazeApp *a, const char *root, const char *rop_path,const char *config_directory) {
    char path[1200]; size_t size,i; uint8_t *bytes;
    const char *slash=strrchr(rop_path,'/'),*backslash=strrchr(rop_path,'\\');
    memset(a,0,sizeof(*a));
    if (strlen(root)>=sizeof(a->root)) { snprintf(a->error,sizeof(a->error),"Asset path too long"); return 0; }
    strcpy(a->root,root);
    if (!blaze_file_read(rop_path,&a->rop,&size) || size!=65536) {
        snprintf(a->error,sizeof(a->error),"Missing/invalid extracted ROP resource"); return 0;
    }
    if(!slash || (backslash && backslash>slash))slash=backslash;
    if(!slash || (size_t)(slash-rop_path)>1100) {
        snprintf(a->error,sizeof(a->error),"Invalid motion resource path");return 0;
    }
    snprintf(path,sizeof(path),"%.*s/trig.bin",(int)(slash-rop_path),rop_path);
    if(!blaze_file_read(path,&bytes,&size)) {
        snprintf(a->error,sizeof(a->error),"Missing motion table");return 0;
    }
    if(size!=1280) {free(bytes);snprintf(a->error,sizeof(a->error),"Invalid motion table");return 0;}
    for(i=0;i<320;++i)a->trig[i]=blaze_signed(blaze_le32(bytes+i*4));
    free(bytes);
    if (!blaze_path(path,sizeof(path),root,"dar/BLZ_TEXT.DAR") ||
        !blaze_archive_read(&a->font,path,a->error,sizeof(a->error))) return 0;
    if (a->font.count!=64) { snprintf(a->error,sizeof(a->error),"Invalid font archive"); return 0; }
    blaze_random_begin(&a->random,(uint32_t)time(NULL));
    a->weapon=1;
    for(i=0;i<BLAZE_MUSIC_GROUPS*3;++i) {BlazeMusicTrack *t=&a->tracks[i/3][i%3];strcpy(t->file,"0");strcpy(t->name,"0");t->cd=1;}
    a->max_stage=5;a->frame_ms=25; /* Original missing-key default, in milliseconds. */
    a->loadout.weapons[0].enabled=a->loadout.weapons[1].enabled=a->loadout.weapons[2].enabled=1;
    {char directory[1200];
    if(!config_directory) {
        if(!blaze_path(directory,sizeof(directory),root,"dat"))return 0;config_directory=directory;
    }
    if(!blaze_path(path,sizeof(path),config_directory,"Blz.ini"))return 0;config(a,path,0);
    if(!blaze_path(path,sizeof(path),config_directory,"BlzScr.ini"))return 0;config(a,path,1);}

    return screen(a,BLAZE_INTRO,"dar/BLZ_TEA.DAR",0);
}
void blaze_app_close(BlazeApp *a) {
    blaze_music_stop(a);if(a->media.silence)a->media.silence(a->media.context);
    blaze_world_end(&a->world);
    blaze_level_close(&a->level_data);
    blaze_archive_free(&a->menu_image);blaze_archive_free(&a->screen); blaze_archive_free(&a->font);blaze_archive_free(&a->game_over_image);
    free(a->rop); a->rop=NULL;memset(&a->media,0,sizeof(a->media));
}
static void text(BlazeApp *a, const char *s, int x, int y, int align) {
    blaze_text(&a->frame,&a->font,s,x,y,align,a->rop);
}
static void title_draw(BlazeApp *a) {
    const char *items[]={"SCRAMBLE","SCORE ATTACK","OPTION","EXIT"};
    char line[80]; int i;
    blaze_draw(&a->frame,&a->screen,0,0,0,NULL);
    for (i=0;i<4;++i) {
        snprintf(line,sizeof(line),a->title_item==i && (a->tick&1) ? "<%s>" : " %s ",items[i]);
        text(a,line,0,128+i*16,4);
    }
    if (a->tick>10) {
        text(a,"VERSION 2.00",0,224,2);
        snprintf(line,sizeof(line)," HI SCORE:%d",a->high_score); text(a,line,0,224,0);
    }
}
static void select_draw(BlazeApp *a) {
    static const char *modes[]={"STANDARD","ONE WEAPON ONLY","WEAPON NO LIMIT AND NO REPAIR","CHANGE WEAPON"};
    char line[80];
    memset(a->frame.pixels,0,sizeof(a->frame.pixels));
    blaze_draw(&a->frame,&a->screen,12,0,0,NULL);
    if (a->select_step==0) {
        text(a,"-SHIP SELECT-",0,0,0);
        snprintf(line,sizeof(line),"TYPE %d",a->ship); text(a,line,32,64,0);
        blaze_draw(&a->frame,&a->screen,(size_t)(13+a->ship),32,96,NULL);
    } else if (a->select_step==1) {
        text(a,"-MODE SELECT-",0,0,0);
        snprintf(line,sizeof(line),"MODE %d",a->mode); text(a,line,32,64,0);
        text(a,modes[a->mode],32,80,0);
    } else {
        text(a,"-WEAPON SELECT-",0,0,0);
        text(a,a->loadout.weapons[a->weapon].name,32,64,0);
        blaze_draw(&a->frame,&a->screen,(size_t)a->weapon,32,96,NULL);
        if (!a->loadout.weapons[a->weapon].enabled) text(a,"NOT USED",32,96,0);
    }
}
static void options_draw(BlazeApp *a) {
    static const char *labels[]={"-GAME LEVEL-","-SCREEN MODE-","-MUSIC-","",""};
    static const char *levels[]={"1:EASY","2:NORMAL","3:HARD"};
    static const char *displays[]={"1:WINDOW 320*240","2:WINDOW 640*480","3:FULL SCALED","4:FULL CENTERED 320*240","5:FULL SCALED 640*480"};
    static const char *music[]={"RANDOM","MIDI:MUSHI","MIDI:UMEO","CD:RANDOM","CD:MIDI:MUSHI","CD:UMEO"};
    const char *values[]={levels[a->level_pending],displays[a->display_pending],music[a->music_pending],"HI-SCORE RANKING","EXIT"};
    int i; char line[80];
    memset(a->frame.pixels,0,sizeof(a->frame.pixels));
    blaze_draw(&a->frame,&a->screen,12,0,0,NULL); text(a,"OPTION",0,0,0);
    for (i=0;i<5;++i) {
        text(a,labels[i],0,48+i*32,0);
        snprintf(line,sizeof(line),i==a->option_item && (a->tick&1) ? "<%s>" : " %s ",values[i]);
        text(a,line,32,64+i*32,0);
    }
}
static void ending_typing(BlazeApp *a,const char *line) {
    size_t i,n=strlen(line),visible=a->clear_characters++/2;
    int width=a->font.sprites[0].width,x=160-(int)n*width/2;
    if(n>visible)n=visible;
    for(i=0;i<n;++i)if(line[i]>=32 && line[i]<=90)
        blaze_draw(&a->frame,&a->font,(unsigned)line[i]-32,x+(int)i*width,112,NULL);
}
/* 0x404310. The app owns each credit archive transactionally; the completed
   world is frozen until exit so score/unlock state has one writable owner.
   Rankings/unlocks stay app-owned; the platform saves awards when they occur. */
static int ending(BlazeApp *a,unsigned actions) {
    static const char *credits[]={"- STAFF -"," ","< MACHINE > SASAKA SITO",
        "< MACHINE > POSITRON","< LOGO > MABOROSHI","< STORY > OOSAMA",
        "< MUSIC > MIDI-MUSHI","< MUSIC > UMEO","< PROGRAM . CG > TEA"};
    static const unsigned bodies[]={0,0x2b4,0x2cc,0x2ae,0x29c};
    unsigned t=a->tick,i;char line[96],path[64];
    memset(a->frame.pixels,0,sizeof(a->frame.pixels));
    if(!t) {a->ending_card=0;a->ending_rank=-1;blaze_music_load(a,5,0);blaze_music_play(a,0,0);}
    if(t<48)text(a,"ALL STAGE CLEAR",0,112,4);
    if(t==18 && a->loadout.mode==4) {
        int score=blaze_signed(a->world.player.words[0x58/4]);
        for(i=0;i<5;++i)if(a->ranking_scores[i]<score) {
            unsigned j;for(j=4;j>i;--j) {
                a->ranking_scores[j]=a->ranking_scores[j-1];a->ranking_modes[j]=a->ranking_modes[j-1];
            }
            a->ranking_scores[i]=score;a->ranking_modes[i]=(int)a->loadout.mode;
            a->ending_rank=(int)i;a->high_score=a->ranking_scores[0];
            if(!blaze_progress_save(a))return 0;break;
        }
    }
    if(t==65) {
        snprintf(path,sizeof(path),"dar/BLZ_MAP%u.DAR",a->ending_card/2+1);
        if(!screen(a,BLAZE_ENDING,path,0))return 0;
        a->tick=t;a->clear_characters=0;
        blaze_color_table(a->game_over_colors,a->screen.bytes+12,0x101010,70);
    }
    if(t>=65 && t<=128 && a->ending_card/2>0)
        blaze_draw_scaled(&a->frame,&a->screen,bodies[a->ending_card/2]-0x170,0,0,256,256,NULL,a->game_over_colors);
    if(t>65 && t<128)ending_typing(a,credits[a->ending_card]);
    if(t==128 && ++a->ending_card<sizeof(credits)/sizeof(credits[0]))a->tick=64;
    if(t>128 && t<180 && a->loadout.mode==4) {
        snprintf(line,sizeof(line),"SCORE:%5d",blaze_signed(a->world.player.words[0x58/4]));text(a,line,0,176,4);
        if(a->ending_rank>=0 && (t&1)) {
            snprintf(line,sizeof(line),"TOP %d SCORE",a->ending_rank+1);text(a,line,0,80,4);
            if(!a->ending_rank)text(a,"LETS INTERNET HISCORE",0,144,4);
        }
    }
    if(t==192) {if(!screen(a,BLAZE_ENDING,"dar/BLZ_END.DAR",0))return 0;a->tick=t;}
    if(t>=192 && t!=256)blaze_draw(&a->frame,&a->screen,0,0,0,NULL);
    if(t==256)a->clear_characters=0;
    if(t>256) {
        ending_typing(a,"FIN");
        for(i=7;i<11;++i)if(!a->loadout.weapons[i].enabled) {
            if(t&1) {snprintf(line,sizeof(line),"%s GET",a->loadout.weapons[i].name);text(a,line,0,224,2);}break;
        }
    }
    /* Original tests confirm after incrementing; music EOF also exits during
       credits. A missing device callback keeps the ending available to input. */
    ++a->tick;
    if((a->media.ended && a->media.ended(a->media.context)) || (a->tick>256 && (actions&BLAZE_CONFIRM))) {
        blaze_music_stop(a);
        for(i=7;i<11;++i)if(!a->loadout.weapons[i].enabled) {
            a->loadout.weapons[i].enabled=1;if(!blaze_progress_save(a))return 0;break;
        }
        blaze_world_end(&a->world);blaze_level_close(&a->level_data);
        if(a->loadout.mode==4) {a->ranking_return=0;return menu(a,BLAZE_RANKING);}
        return screen(a,BLAZE_INTRO,"dar/BLZ_TEA.DAR",0);
    }
    return 1;
}
int blaze_app_tick(BlazeApp *a, unsigned input) {
    unsigned control=input&(BLAZE_PAUSE|BLAZE_MAIN|BLAZE_QUIT),edge=control&~a->previous_control;
    a->previous_control=control;input&=~(BLAZE_PAUSE|BLAZE_MAIN|BLAZE_QUIT);
    if(edge&BLAZE_QUIT) {blaze_music_stop(a);if(a->media.silence)a->media.silence(a->media.context);a->state=BLAZE_EXIT;return 1;}
    if(edge&BLAZE_MAIN) {
        blaze_music_stop(a);if(a->media.silence)a->media.silence(a->media.context);
        blaze_app_pause(a,~0u,0);
        blaze_archive_free(&a->menu_image);blaze_archive_free(&a->game_over_image);blaze_world_end(&a->world);blaze_level_close(&a->level_data);
        a->previous_input=0;return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);
    }
    if(edge&BLAZE_PAUSE) {blaze_app_pause(a,2,!(a->pause_flags&2));return 1;}
    if(a->pause_flags)return 1;
    /* 0x403d70 suppresses a repeated entire mask (not per-bit edges). */
    unsigned actions=input==a->previous_input ? 0 : input;
    a->previous_input=input;
    if (a->state==BLAZE_EXIT || a->state==BLAZE_BLOCKED) return 1;
    if (a->state==BLAZE_INTRO) {
        int phase=(int)a->tick;
        if (phase>=16) phase=31-phase;
        memset(a->frame.pixels,0,sizeof(a->frame.pixels));
        blaze_draw(&a->frame,&a->screen,(size_t)(phase<0?0:phase/4),80,64,NULL);
        if (++a->tick==32) return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);
    } else if (a->state==BLAZE_TITLE) {
        if(!a->tick) {blaze_music_load(a,1,0);blaze_music_play(a,0,0);}
        if(a->tick<2)memset(a->frame.pixels,0,sizeof(a->frame.pixels));
        ++a->tick;if(a->tick>2)title_draw(a);
        if(a->media.ended && a->media.ended(a->media.context)) {
            blaze_music_stop(a);a->ranking_return=0;return menu(a,BLAZE_RANKING);
        }
        if (a->tick<=4) return 1;
        if(actions&(BLAZE_UP|BLAZE_DOWN))if(a->media.sound)a->media.sound(a->media.context,BLAZE_HIT);
        if (actions&BLAZE_UP) a->title_item=clamp(a->title_item-1,0,3);
        if (actions&BLAZE_DOWN) a->title_item=clamp(a->title_item+1,0,3);
        if (actions&BLAZE_CONFIRM) {
            blaze_music_stop(a);if(a->media.sound)a->media.sound(a->media.context,BLAZE_PUU);
            if (a->title_item==3) { a->state=BLAZE_EXIT; return 1; }
            if (a->title_item==2) return menu(a,BLAZE_OPTIONS);
            a->ship=0; a->mode=a->title_item==1?4:0; a->weapon=1;
            a->select_step=a->title_item==1?2:0;
            return menu(a,BLAZE_SELECT);
        }
    } else if (a->state==BLAZE_SELECT) {
        int *selection=a->select_step==0?&a->ship:a->select_step==1?&a->mode:&a->weapon;
        int low=a->select_step==2?1:0, high=a->select_step==0?2:a->select_step==1?3:10;
        if(!blaze_menu_transition(a))return 0;
        if(a->tick==16) {memcpy(a->frame.palette,a->screen.bytes+12,1024);blaze_music_load(a,2,0);blaze_music_play(a,0,1);}
        else if(a->tick>15)select_draw(a);
        ++a->tick;
        if (actions&(BLAZE_DOWN|BLAZE_RIGHT)) {*selection=clamp(*selection+1,low,high);sound(a,BLAZE_HIT);}
        if (actions&(BLAZE_UP|BLAZE_LEFT)) {*selection=clamp(*selection-1,low,high);sound(a,BLAZE_HIT);}
        /* Original cancel overrides confirm's deferred transition. Resolve it
           before eagerly constructing a native world on a combined mask. */
        if(actions&BLAZE_CONFIRM)sound(a,BLAZE_PUU);
        if (actions&BLAZE_CANCEL) {sound(a,BLAZE_PUU);blaze_music_stop(a);blaze_archive_free(&a->menu_image);return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);}
        if (actions&BLAZE_CONFIRM) {
            if (a->select_step<2) ++a->select_step;
            else if (a->loadout.weapons[a->weapon].enabled) {
                if (!blaze_level_open(&a->level_data,a->root,0,(unsigned)a->level,a->error,sizeof(a->error))) return 0;
                a->loadout.ship=(unsigned)a->ship;a->loadout.mode=(unsigned)a->mode;a->loadout.weapon=(unsigned)a->weapon;
                if(!blaze_world_begin(&a->world,&a->level_data,&a->font,&a->loadout,&a->random,a->trig)) {
                    snprintf(a->error,sizeof(a->error),"Cannot construct player/pools");return 0;
                }
                a->world.media=&a->media;blaze_music_stop(a);
                blaze_archive_free(&a->screen);blaze_archive_free(&a->menu_image);
                memcpy(a->frame.palette,a->level_data.tiles.bytes+12,1024);
                a->state=BLAZE_LOADING;a->tick=0;
            }
        }
    } else if(a->state==BLAZE_LOADING) {
        char label[32];
        if(a->restart_pending && a->tick==1) {
            unsigned stage=a->pending_stage,difficulty=a->level_data.difficulty;
            if(!blaze_level_open(&a->level_data,a->root,stage,difficulty,a->error,sizeof(a->error)))return 0;
            if(!blaze_world_restart(&a->world)) {
                snprintf(a->error,sizeof(a->error),"Cannot reset continued player/pools");return 0;
            }
            a->restart_pending=0;memcpy(a->frame.palette,a->level_data.tiles.bytes+12,1024);
        }memset(a->frame.pixels,0,sizeof(a->frame.pixels));
        text(a,"NOW LOADING",0,224,0);snprintf(label,sizeof(label),"STAGE %u",a->level_data.stage+1);
        text(a,label,0,112,4);
        if(++a->tick==4) {a->state=BLAZE_START;a->tick=0;}
    } else if(a->state==BLAZE_START || a->state==BLAZE_GAME) {
        if(input&BLAZE_ABORT) {
            blaze_music_stop(a);if(a->media.silence)a->media.silence(a->media.context);
            blaze_world_end(&a->world);blaze_level_close(&a->level_data);
            return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);
        }
        if(a->state==BLAZE_START && !a->tick) {
            blaze_music_load(a,7+a->level_data.stage,0);blaze_music_load(a,12+a->level_data.stage,1);
            if(a->level_data.stage+1<a->max_stage)blaze_music_play(a,0,1);
        }
        if(!blaze_world_step(&a->world,a->trig,input,a->error,sizeof(a->error))) {a->state=BLAZE_BLOCKED;return 1;}
        blaze_world_draw(&a->world,&a->frame,a->rop);
        if(a->world.requested_screen==4) {a->state=BLAZE_STAGE_CLEAR;a->tick=0;}
        if(a->world.dying && a->state==BLAZE_GAME) {
            a->state=BLAZE_GAME_OVER;a->tick=0;a->game_over_caption=0;
        }
        if(a->state==BLAZE_START) {
            text(a,"START",0,112,4);
            if(++a->tick>32) {a->state=BLAZE_GAME;a->tick=0;}
        }
    } else if(a->state==BLAZE_STAGE_CLEAR) {
        if(input&BLAZE_ABORT) {blaze_music_stop(a);if(a->media.silence)a->media.silence(a->media.context);blaze_world_end(&a->world);blaze_level_close(&a->level_data);return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);}
        if(!blaze_stage_clear_tick(a,input))return 0;
    } else if(a->state==BLAZE_ENDING) {
        return ending(a,actions);
    } else if(a->state==BLAZE_GAME_OVER) {
        if(input&BLAZE_ABORT) {
            blaze_music_stop(a);if(a->media.silence)a->media.silence(a->media.context);
            blaze_archive_free(&a->menu_image);blaze_archive_free(&a->game_over_image);blaze_world_end(&a->world);blaze_level_close(&a->level_data);
            return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);
        }
        if(!blaze_game_over_tick(a,input))return 0;
        if(a->state==BLAZE_TITLE) {
            blaze_world_end(&a->world);blaze_level_close(&a->level_data);
            return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);
        }
    } else if (a->state==BLAZE_OPTIONS) {
        if(!blaze_menu_transition(a))return 0;
        if(a->tick==16) {
            memcpy(a->frame.palette,a->screen.bytes+12,1024);
            blaze_music_load(a,3,0);blaze_music_play(a,0,1);
            a->level_pending=a->level;a->display_pending=a->display;a->music_pending=a->music;
        } else if(a->tick>15)options_draw(a);
        ++a->tick;
        if(actions&BLAZE_DOWN) {a->option_item=clamp(a->option_item+1,0,4);sound(a,BLAZE_HIT);}
        if(actions&BLAZE_UP) {a->option_item=clamp(a->option_item-1,0,4);sound(a,BLAZE_HIT);}
        if(actions&(BLAZE_LEFT|BLAZE_RIGHT)) {
            int *value=a->option_item==0?&a->level_pending:a->option_item==1?&a->display_pending:&a->music_pending;
            int maximum=a->option_item==0?2:a->option_item==1?4:5;
            if(actions&BLAZE_LEFT) {if(a->option_item<3)*value=clamp(*value-1,0,maximum);sound(a,BLAZE_HIT);}
            if(actions&BLAZE_RIGHT) {if(a->option_item<3)*value=clamp(*value+1,0,maximum);sound(a,BLAZE_HIT);}
        }
        if(actions&BLAZE_CONFIRM) {
            blaze_music_stop(a);sound(a,BLAZE_PUU);
            if(a->option_item==1 && (!display(a,a->display_pending) || !save_options(a)))return 0;
            if(a->option_item<3)blaze_music_play(a,0,1);
            else if(a->option_item==3 && !(actions&BLAZE_CANCEL)) {
                a->ranking_return=1;return menu(a,BLAZE_RANKING);
            } else if(a->option_item==4) {
                a->level=a->level_pending;a->music=a->music_pending;
                if(a->display!=a->display_pending && !display(a,a->display_pending))return 0;
                if(!save_options(a))return 0;
            }
        }
        if((actions&BLAZE_CANCEL) || ((actions&BLAZE_CONFIRM) && a->option_item==4)) {
            if(actions&BLAZE_CANCEL) {blaze_music_stop(a);sound(a,BLAZE_PUU);}
            blaze_archive_free(&a->menu_image);
            return screen(a,BLAZE_TITLE,"dar/blz_opn.dar",0);
        }
    } else if(a->state==BLAZE_RANKING) {
        static const char *ranks[]={"TOP","2ND","3RD","4TH","5TH"};
        unsigned i;char line[80];
        if(!blaze_menu_transition(a))return 0;
        if(a->tick==16) {
            memcpy(a->frame.palette,a->screen.bytes+12,1024);
            blaze_music_load(a,4,0);blaze_music_play(a,0,0);
        } else if(a->tick>15) {
            blaze_draw(&a->frame,&a->screen,12,0,0,NULL);text(a,"HI SCORE RANKING",0,0,0);
            for(i=0;i<5;++i) {
                snprintf(line,sizeof(line),"< %s %5d >",ranks[i],a->ranking_scores[i]);
                text(a,line,0,64+(int)i*32,4);
            }
        }
        ++a->tick;
        if((actions&(BLAZE_CONFIRM|BLAZE_CANCEL)) ||
           (a->tick>16 && a->media.ended && a->media.ended(a->media.context))) {
            blaze_music_stop(a);sound(a,BLAZE_PUU);
            if(a->ranking_return)return menu(a,BLAZE_OPTIONS);
            blaze_archive_free(&a->menu_image);
            /* Dispatcher state 12 reuses the 32-tick tea intro before title. */
            return screen(a,BLAZE_INTRO,"dar/BLZ_TEA.DAR",0);
        }
    }
    return 1;
}
