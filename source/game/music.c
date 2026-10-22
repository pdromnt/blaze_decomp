#include "blaze.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void group_name(unsigned group,char out[32],int credit) {
    static const char *names[]={"Start","Openning","Select","Option","Ranking","Ending","GameOver"};
    if(group<7)snprintf(out,32,"%s%s",names[group],credit?"Name":"");
    else if(group<12)snprintf(out,32,credit?"StageName%u":"Stage%u",group-6);
    else snprintf(out,32,credit?"StageBossName%u":"StageBoss%u",group-11);
}
void blaze_music_key(BlazeApp *a,const char *key,const char *value) {
    unsigned g,c;
    for(g=0;g<BLAZE_MUSIC_GROUPS;++g)for(c=0;c<2;++c) {
        char base[32],expected[48];BlazeMusicTrack *t=&a->tracks[g][c];
        group_name(g,base,0);snprintf(expected,sizeof(expected),"%s%u",base,c+1);
        /* Win32 INI lookup takes the first duplicate, notably Option1/Option2. */
        if(!strcmp(key,expected) && !t->file_set) {snprintf(t->file,80,"%.79s",value);t->file_set=1;}
        snprintf(expected,sizeof(expected),"%sCD%u",base,c+1);
        if(!strcmp(key,expected))t->cd=(unsigned)strtoul(value,NULL,10);
        group_name(g,base,1);snprintf(expected,sizeof(expected),"%s%u",base,c+1);
        if(!strcmp(key,expected))snprintf(t->name,80,"%.79s",value);
    }
}
void blaze_app_set_media(BlazeApp *a,const BlazeMedia *media) {
    if(media)a->media=*media;else memset(&a->media,0,sizeof(a->media));
    if(a->world.active)a->world.media=&a->media;
}
void blaze_music_load(BlazeApp *a,unsigned group,unsigned slot) {
    unsigned choice;BlazeMusicTrack *t;
    if(group>=BLAZE_MUSIC_GROUPS || slot>1)return;
    /* Preserve the inclusive RNG endpoint (choice 2 is a missing third INI key).
       Muting or lacking a device must never change the game's random stream. */
    choice=a->music%3==0?blaze_random_next(&a->random)*2/32767:(unsigned)(a->music%3-1);
    a->music_group[slot]=group;a->music_choice[slot]=choice;t=&a->tracks[group][choice];
    if(group>=12)snprintf(a->loadout.boss_music[group-12],80,"%s",t->name);
    if(group>=7 && group<12)snprintf(a->stage_music_name,80,"%s",t->name);
    if(a->media.load)a->media.load(a->media.context,slot,t->file,t->cd,a->music>=3);
}
void blaze_music_play(BlazeApp *a,unsigned slot,int loop) {
    if(a->media.play)a->media.play(a->media.context,slot,loop);
}
void blaze_music_stop(BlazeApp *a) {if(a->media.stop)a->media.stop(a->media.context);}
void blaze_app_pause(BlazeApp *a,unsigned reason,int paused) {
    unsigned before=a->pause_flags;
    if(paused)a->pause_flags|=reason;else a->pause_flags&=~reason;
    if(!(before&2) && (a->pause_flags&2)) {
        blaze_text(&a->frame,&a->font,"< PAUSE >",0,80,4,a->rop);
        blaze_text(&a->frame,&a->font,"[ESC]CONT [F2]MAIN [F9]EXIT",0,112,4,a->rop);
    }
    /* Original 0x401110/0x4176e0 stop simulation, not asynchronous audio. */
}
