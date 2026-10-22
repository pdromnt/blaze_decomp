#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "settings.h"
#include "blaze_files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

int blaze_settings_paths(BlazeSettings *s,const char *directory) {
    s->path[0]=s->score_path[0]=s->controls_path[0]=0;
    if(!directory)return 1;
    return blaze_path(s->path,sizeof(s->path),directory,"Blz.ini") &&
        blaze_path(s->score_path,sizeof(s->score_path),directory,"BlzScr.ini") &&
        blaze_path(s->controls_path,sizeof(s->controls_path),directory,"BlzCtrl.ini");
}
int blaze_settings_load(BlazeSettings *s,BlazeApp *a) {
    char line[160],key[40],raw[80],section[40];FILE *f;unsigned kind;
    const char *paths[]={s->path,s->score_path,s->controls_path};
    controls_defaults(&s->controls);
    for(kind=0;kind<3;++kind)if(paths[kind][0]) {
        section[0]=0;f=blaze_fopen(paths[kind],"rb");if(!f && errno!=ENOENT)return 0;
        if(f) {
            while(fgets(line,sizeof(line),f)) {
                long value;char *end;unsigned i;
                if(sscanf(line,"[%39[^]]]",section)==1)continue;
                if(sscanf(line,"%39[^=]=%79[^\r\n]",key,raw)!=2)continue;
                errno=0;value=strtol(raw,&end,10);
                if(end==raw || errno==ERANGE || value<INT_MIN || value>INT_MAX)continue;
                while(isspace((unsigned char)*end))++end;
                if(*end)continue;
                if(kind==0 && !strcmp(section,"Enh")) {
                    if(!strcmp(key,"Level") && value>=0 && value<=2)a->level=(int)value;
                    if(!strcmp(key,"Screen") && value>=0 && value<=4)a->display=(int)value;
                    if(!strcmp(key,"MidPlayer") && value>=0 && value<=5)a->music=(int)value;
                } else if(kind==2 && !strcmp(section,"Input"))controls_key(&s->controls,key,(int)value);
                else if(kind==1 && !strcmp(section,"Score")) {
                    char expected[32];
                    for(i=0;i<5;++i) {snprintf(expected,sizeof(expected),"HiScore%u",i+1);
                        if(!strcmp(key,expected) && value>=0)a->ranking_scores[i]=(int)value;}
                    for(i=0;i<5;++i) {snprintf(expected,sizeof(expected),"ClearMode%u",i+1);
                        if(!strcmp(key,expected))a->ranking_modes[i]=(int)value;}
                    for(i=0;i<11;++i) {snprintf(expected,sizeof(expected),"WeaponFlag%u",i);
                        if(!strcmp(key,expected) && (value==0 || value==1))a->loadout.weapons[i].enabled=(int)value;}
                }
            }
            if(ferror(f)) {fclose(f);return 0;}if(fclose(f))return 0;
        }
    }
    a->high_score=a->ranking_scores[0];return 1;
}

typedef struct {char key[32];int value,found;} IniValue;
static int span_equal(const uint8_t *bytes,size_t size,const char *text) {
    return strlen(text)==size && !memcmp(bytes,text,size);
}
static int emit_values(FILE *f,IniValue *values,unsigned count) {
    unsigned i;for(i=0;i<count;++i)if(!values[i].found) {
        if(fprintf(f,"%s=%d\r\n",values[i].key,values[i].value)<0)return 0;
        values[i].found=1;
    }
    return 1;
}
/* One transaction per INI. Unknown bytes/comments/order survive unchanged.
   Owned duplicate keys are all updated so a later valid duplicate cannot undo
   the save. No profile cache, serialized state mirror or app pointer is retained. */
static int update_ini(const char *path,const char *section,IniValue *values,unsigned count) {
    uint8_t *bytes=NULL;size_t size=0,offset=0;char temporary[1280];FILE *f=NULL,*input;
    int ok=0,in_section=0,seen_section=0,owned=0;unsigned attempt;
    if(!*path)return 1;
    input=blaze_fopen(path,"rb");
    if(input) {
        long length;
        if(fseek(input,0,SEEK_END) || (length=ftell(input))<0 || length>4*1024*1024 || fseek(input,0,SEEK_SET)) {fclose(input);return 0;}
        size=(size_t)length;bytes=malloc(size?size:1);
        if(!bytes) {fclose(input);return 0;}
        if(fread(bytes,1,size,input)!=size) {fclose(input);free(bytes);return 0;}
        if(fclose(input)) {free(bytes);return 0;}
    } else if(errno!=ENOENT)return 0;
    for(attempt=0;attempt<16;++attempt) {
#ifdef _WIN32
        unsigned long pid=(unsigned long)GetCurrentProcessId();
#else
        unsigned long pid=(unsigned long)getpid();
#endif
        if(snprintf(temporary,sizeof(temporary),"%s.%lu.%u.tmp",path,pid,attempt)>=(int)sizeof(temporary))goto done;
        f=blaze_fopen(temporary,"wbx");if(f) {owned=1;break;}if(errno!=EEXIST)goto done;
    }
    if(!f)goto done;
    while(offset<size) {
        size_t start=offset,end,content;unsigned i;int replaced=0;
        while(offset<size && bytes[offset]!='\n')++offset;
        if(offset<size)++offset;
        end=offset;content=end;
        while(content>start && (bytes[content-1]=='\r' || bytes[content-1]=='\n'))--content;
        if(content>start && bytes[start]=='[') {
            size_t close=start+1;while(close<content && bytes[close]!=']')++close;
            if(close<content) {
                if(in_section && !emit_values(f,values,count))goto done;
                in_section=span_equal(bytes+start+1,close-start-1,section);
                if(in_section)seen_section=1;
            }
        } else if(in_section) {
            size_t equal=start;while(equal<content && bytes[equal]!='=')++equal;
            for(i=0;i<count;++i)if(equal<content && span_equal(bytes+start,equal-start,values[i].key)) {
                if(fprintf(f,"%s=%d\r\n",values[i].key,values[i].value)<0)goto done;
                values[i].found=1;replaced=1;break;
            }
        }
        if(!replaced && fwrite(bytes+start,1,end-start,f)!=end-start)goto done;
    }
    if(size && bytes[size-1]!='\n' && fputs("\r\n",f)<0)goto done;
    if(!seen_section && fprintf(f,"[%s]\r\n",section)<0)goto done;
    /* If the last section was foreign, open the owned section before appending. */
    if(seen_section && !in_section) {
        unsigned i;int missing=0;for(i=0;i<count;++i)missing|=!values[i].found;
        if(missing && fprintf(f,"[%s]\r\n",section)<0)goto done;
    }
    if(!emit_values(f,values,count) || fflush(f))goto done;
#ifdef _WIN32
    if(_commit(_fileno(f)))goto done;
#else
    if(fsync(fileno(f)))goto done;
#endif
    {int closed=fclose(f);f=NULL;if(closed)goto done;}
    ok=blaze_file_replace(temporary,path);
done:
    if(f)fclose(f);
    if(!ok && owned)blaze_file_remove(temporary);
    free(bytes);return ok;
}
int blaze_settings_options(BlazeSettings *s,int level,int display,int music) {
    IniValue values[]={{"Level",level,0},{"Screen",display,0},{"MidPlayer",music,0}};
    return update_ini(s->path,"Enh",values,3);
}
int blaze_settings_progress(BlazeSettings *s,const int scores[5],const int modes[5],unsigned weapons) {
    IniValue values[21]={0};unsigned i;
    for(i=0;i<5;++i) {
        snprintf(values[i*2].key,32,"HiScore%u",i+1);values[i*2].value=scores[i];
        snprintf(values[i*2+1].key,32,"ClearMode%u",i+1);values[i*2+1].value=modes[i];
    }
    for(i=0;i<11;++i) {snprintf(values[10+i].key,32,"WeaponFlag%u",i);values[10+i].value=(weapons>>i)&1;}
    return update_ini(s->score_path,"Score",values,21);
}
int blaze_settings_controls(BlazeSettings *s) {
    IniValue values[2+BLAZE_BINDINGS*2]={{"PadDevice",s->controls.device,0},{"Deadzone",s->controls.deadzone,0}};unsigned i;
    for(i=0;i<BLAZE_BINDINGS;++i) {
        snprintf(values[2+i*2].key,32,"%s",binding_keys[i]);values[2+i*2].value=s->controls.keys[i];
        snprintf(values[3+i*2].key,32,"%s",binding_buttons[i]);values[3+i*2].value=s->controls.buttons[i];
    }
    return update_ini(s->controls_path,"Input",values,2+BLAZE_BINDINGS*2);
}
