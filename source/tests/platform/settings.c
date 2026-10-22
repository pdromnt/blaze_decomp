#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"settings line %d: %s\n",__LINE__,#x);goto done;}} while(0)
static int write_file(const char *path,const char *text) {
    FILE *f=fopen(path,"wb");int ok;if(!f)return 0;ok=fputs(text,f)>=0;return fclose(f)==0 && ok;
}
static int contains(const char *path,const char *text) {
    char bytes[4096]={0};FILE *f=fopen(path,"rb");size_t count;int ok;
    if(!f)return 0;count=fread(bytes,1,sizeof(bytes)-1,f);ok=!ferror(f);fclose(f);
    return ok && count && strstr(bytes,text)!=NULL;
}
int main(int argc,char **argv) {
    BlazeSettings s={0},disabled={0};BlazeApp *a=calloc(1,sizeof(*a));
    int scores[]={7000,6000,5000,4000,3000},modes[]={4,1,2,3,0};
    char collision[1280];unsigned i,created=0;int result=1;
#ifdef _WIN32
    unsigned long pid=(unsigned long)GetCurrentProcessId();
#else
    unsigned long pid=(unsigned long)getpid();
#endif
    CHECK(argc==2 && a && blaze_settings_paths(&s,argv[1]));
    CHECK(write_file(s.path,";untouched\r\n[Enh]\r\nTime=40\r\nLevel=1\r\nLevel=2\r\nWeaponName4=LIGHTNING\r\n[Foreign]\r\nLevel=77\r\nTail=no-newline"));
    CHECK(blaze_settings_options(&s,0,4,2));
    CHECK(blaze_settings_progress(&s,scores,modes,0x401));
    controls_defaults(&s.controls);controls_key(&s.controls,"KeyFire",'A');controls_key(&s.controls,"ButtonFire",32);
    CHECK(blaze_settings_controls(&s) && blaze_settings_load(&s,a));
    CHECK(a->level==0 && a->display==4 && a->music==2 && a->high_score==7000);
    CHECK(!memcmp(a->ranking_scores,scores,sizeof(scores)) && !memcmp(a->ranking_modes,modes,sizeof(modes)));
    CHECK(a->loadout.weapons[0].enabled && a->loadout.weapons[10].enabled && !a->loadout.weapons[1].enabled);
    CHECK(s.controls.keys[4]=='A' && s.controls.buttons[4]==32);
    CHECK(contains(s.path,";untouched\r\n") && contains(s.path,"Time=40\r\n") && contains(s.path,"WeaponName4=LIGHTNING\r\n"));
    CHECK(contains(s.path,"[Foreign]\r\nLevel=77\r\nTail=no-newline"));
    CHECK(!contains(s.path,"Level=1\r\n") && !contains(s.path,"Level=2\r\n"));
    CHECK(blaze_settings_paths(&disabled,NULL) && blaze_settings_options(&disabled,1,1,1));
    /* Exhausting temp-name candidates must preserve every pre-existing file.
       The writer owns only a file it successfully created exclusively. */
    for(i=0;i<16;++i) {
        snprintf(collision,sizeof(collision),"%s.%lu.%u.tmp",s.score_path,pid,i);
        CHECK(write_file(collision,"existing-owner"));++created;
    }
    scores[0]=8000;CHECK(!blaze_settings_progress(&s,scores,modes,0));
    for(i=0;i<created;++i) {
        snprintf(collision,sizeof(collision),"%s.%lu.%u.tmp",s.score_path,pid,i);
        CHECK(contains(collision,"existing-owner"));
    }
    CHECK(blaze_settings_load(&s,a) && a->high_score==7000);
    puts("Shared INI ownership, duplicates, foreign bytes, score/mode reload and exclusive-temp failure passed");result=0;
done:
    for(i=0;i<created;++i) {snprintf(collision,sizeof(collision),"%s.%lu.%u.tmp",s.score_path,pid,i);remove(collision);}
    if(s.path[0])remove(s.path);if(s.score_path[0])remove(s.score_path);if(s.controls_path[0])remove(s.controls_path);
    free(a);return result;
}
