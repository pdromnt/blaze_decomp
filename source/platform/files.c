#include "blaze_files.h"
#include <errno.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
static int wide(wchar_t *out,size_t count,const char *text) {
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,out,(int)count)) {errno=EINVAL;return 0;}return 1;
}
FILE *blaze_fopen(const char *path,const char *mode) {
    wchar_t name[4096],flags[16];if(!wide(name,4096,path) || !wide(flags,16,mode))return NULL;
    return _wfopen(name,flags);
}
int blaze_file_replace(const char *from,const char *to) {
    wchar_t a[4096],b[4096];return wide(a,4096,from) && wide(b,4096,to) && MoveFileExW(a,b,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}
int blaze_file_remove(const char *path) {wchar_t name[4096];return wide(name,4096,path)?_wremove(name):-1;}
#else
#include <dirent.h>
FILE *blaze_fopen(const char *path,const char *mode) {return fopen(path,mode);}
int blaze_file_replace(const char *from,const char *to) {return rename(from,to)==0;}
int blaze_file_remove(const char *path) {return remove(path);}
static int same_name(const char *a,const char *b) {
    for(;*a && *b;++a,++b) {
        unsigned char x=(unsigned char)*a,y=(unsigned char)*b;
        if(x>='A' && x<='Z')x+='a'-'A';if(y>='A' && y<='Z')y+='a'-'A';
        if(x!=y)return 0;
    }return *a==*b;
}
#endif
int blaze_asset_path(char *out,size_t capacity,const char *path) {
    if(strlen(path)>=capacity) {errno=ENAMETOOLONG;return 0;}
#ifdef _WIN32
    strcpy(out,path);return 1;
#else
    const char *cursor=path;size_t used=0;
    if(*cursor=='/') {out[used++]='/';++cursor;}out[used]=0;
    while(*cursor) {
        char part[256],match[256];size_t length=0;DIR *dir;struct dirent *entry;int found=0,exact=0;
        while(*cursor=='/')++cursor;if(!*cursor)break;
        while(cursor[length] && cursor[length]!='/')++length;
        if(length>=sizeof(part)) {errno=ENAMETOOLONG;return 0;}
        memcpy(part,cursor,length);part[length]=0;cursor+=length;
        if(!strcmp(part,".") || !strcmp(part,"..")) {strcpy(match,part);found=1;}
        else {
            dir=opendir(used?out:".");if(!dir)return 0;
            while((entry=readdir(dir))!=NULL)if(same_name(entry->d_name,part)) {
                if(!strcmp(entry->d_name,part)) {strcpy(match,part);exact=1;break;}
                if(!found)strcpy(match,entry->d_name);++found;
            }
            closedir(dir);
        }
        if(!exact && found!=1) {
            if(found)fprintf(stderr,"Ambiguous asset filename case: %s\n",path);
            errno=found?EEXIST:ENOENT;return 0;
        }
        length=strlen(match);
        if(used && out[used-1]!='/') {if(used+1>=capacity)return 0;out[used++]='/';}
        if(length>=capacity-used) {errno=ENAMETOOLONG;return 0;}
        memcpy(out+used,match,length+1);used+=length;
    }
    return used!=0;
#endif
}
FILE *blaze_asset_open(const char *path) {
    FILE *file=blaze_fopen(path,"rb");char actual[4096];
    if(file || errno!=ENOENT)return file;
    if(!blaze_asset_path(actual,sizeof(actual),path))return NULL;
    return blaze_fopen(actual,"rb");
}
