#include "sdl_paths.h"
#include "blaze_files.h"
#include <SDL3/SDL.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
static int seed(const char *assets,const char *directory,const char *name) {
    char from[1200],to[1200],relative[64];uint8_t *bytes=NULL;size_t size=0;FILE *f;int ok;
    if(!blaze_path(to,sizeof(to),directory,name))return 0;
    f=blaze_fopen(to,"rb");if(f) {fclose(f);return 1;}if(errno!=ENOENT)return 0;
    snprintf(relative,sizeof(relative),"dat/%s",name);
    if(!blaze_path(from,sizeof(from),assets,relative) || !blaze_file_read(from,&bytes,&size))return 0;
    f=blaze_fopen(to,"wbx");if(!f) {free(bytes);return errno==EEXIST;}
    ok=fwrite(bytes,1,size,f)==size;if(fclose(f))ok=0;
    if(!ok)blaze_file_remove(to);free(bytes);return ok;
}
int blaze_sdl_paths(BlazeSettings *s,const char *assets,const char *requested,int hidden,int portable) {
    char directory[1200];char *preferences=NULL;int ok;
    if(requested && !strcmp(requested,"none"))return blaze_settings_paths(s,NULL);
    if(!requested && hidden)return blaze_settings_paths(s,NULL);
    if(!requested) {
#ifdef __APPLE__
        if(portable) {SDL_SetError("Use --settings with a writable directory outside the app bundle");return 0;}
#endif
#ifdef _WIN32
        portable=1;
#endif
        if(portable)ok=blaze_path(directory,sizeof(directory),SDL_GetBasePath(),"dat");
        else {
            preferences=SDL_GetPrefPath("BlazeDecomp","BLAZE");
            ok=preferences && blaze_path(directory,sizeof(directory),preferences,"dat");SDL_free(preferences);
        }
        if(!ok)return 0;requested=directory;
    }
    if(!SDL_CreateDirectory(requested) || !seed(assets,requested,"Blz.ini") || !seed(assets,requested,"BlzScr.ini"))return 0;
    return blaze_settings_paths(s,requested);
}
