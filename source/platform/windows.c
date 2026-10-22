#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "blaze.h"
#include "windows_audio.h"
#include "windows_resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include "windows_input.h"

/* Original 0x402a93 -> 0x4179b0 passes INI Time as milliseconds,
   not Hz. Keep tick deadlines independent of work duration, without replaying
   a backlog after a slow frame, focus loss or minimize. */
typedef struct {ULONGLONG next;unsigned period;} FrameClock;
/* GetTickCount64 advances in coarse (~15 ms) steps even when a short wait
   expires. QPC preserves the actual deadlines rather than alternating short
   and long displayed frames while reporting the correct average tick rate. */
static ULONGLONG clock_ms(const LARGE_INTEGER *frequency) {
    LARGE_INTEGER now;QueryPerformanceCounter(&now);
    return (ULONGLONG)(now.QuadPart/frequency->QuadPart)*1000+
        (ULONGLONG)(now.QuadPart%frequency->QuadPart)*1000/(ULONGLONG)frequency->QuadPart;
}
static DWORD frame_wait(const FrameClock *clock,ULONGLONG now) {
    return now<clock->next?(DWORD)(clock->next-now):0;
}
static void frame_advance(FrameClock *clock,ULONGLONG now) {
    clock->next+=clock->period;
    if(clock->next<now)clock->next=now;
}
typedef struct {
    HWND window;
    int mode;
    RECT windowed;
    BlazeControls controls;
    char path[1200],score_path[1200],controls_path[1200];
} WindowSettings;
static int settings_location(WindowSettings *s,const char *directory,const char *requested,int hidden) {
    char folder[1200],absolute[1200];DWORD length;s->path[0]=s->score_path[0]=s->controls_path[0]=0;
    if(requested) {
        if(!strcmp(requested,"none"))return 1;
        if(strlen(requested)>=sizeof(folder))return 0;strcpy(folder,requested);
    } else {
        if(hidden)return 1;
        if(!blaze_path(folder,sizeof(folder),directory,"dat"))return 0;
    }
    /* Profile APIs otherwise search the Windows directory for relative names. */
    length=GetFullPathNameA(folder,sizeof(absolute),absolute,NULL);
    if(!length || length>=sizeof(absolute))return 0;
    return blaze_path(s->path,sizeof(s->path),absolute,"Blz.ini") &&
        blaze_path(s->score_path,sizeof(s->score_path),absolute,"BlzScr.ini") &&
        blaze_path(s->controls_path,sizeof(s->controls_path),absolute,"BlzCtrl.ini");
}
/* Native full-screen modes are borderless, never change desktop resolution.
   The two legacy 320 modes retain scaled versus centered presentation. */
static int apply_display(void *context,int mode) {
    WindowSettings *s=context;RECT bounds;DWORD style;MONITORINFO monitor={sizeof(monitor)};
    if(mode<0 || mode>4)return 0;
    if(mode<2) {
        int scale=mode+1;bounds.left=bounds.top=0;bounds.right=320*scale;bounds.bottom=240*scale;
        style=WS_OVERLAPPEDWINDOW;AdjustWindowRect(&bounds,style,FALSE);
        if(s->mode>=2 && !SetWindowPos(s->window,NULL,s->windowed.left,s->windowed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE))return 0;
        SetLastError(0);if(!SetWindowLongPtrA(s->window,GWL_STYLE,style) && GetLastError())return 0;
        if(!SetWindowPos(s->window,NULL,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top,
            SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED))return 0;
    } else {
        if(s->mode<2 && !GetWindowRect(s->window,&s->windowed))return 0;
        if(!GetMonitorInfoA(MonitorFromWindow(s->window,MONITOR_DEFAULTTONEAREST),&monitor))return 0;
        bounds=monitor.rcMonitor;
        SetLastError(0);if(!SetWindowLongPtrA(s->window,GWL_STYLE,WS_POPUP) && GetLastError())return 0;
        if(!SetWindowPos(s->window,NULL,bounds.left,bounds.top,bounds.right-bounds.left,bounds.bottom-bounds.top,
            SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED))return 0;
    }
    s->mode=mode;InvalidateRect(s->window,NULL,FALSE);return 1;
}
/* Edit an owned temporary snapshot, preserving original keys, track mappings,
   weapon definitions, ClearMode/ID and foreign sections. Each owner writes only
   its own INI; the atomic replacement keeps a failed write from damaging it. */
static int ini_begin(const char *path,char temporary[1240]) {
    FILE *file;
    snprintf(temporary,1240,"%s.%lu.tmp",path,(unsigned long)GetCurrentProcessId());
    if(CopyFileA(path,temporary,FALSE))return 1;
    if(GetLastError()!=ERROR_FILE_NOT_FOUND)return 0;
    file=fopen(temporary,"wb");if(!file)return 0;
    if(fclose(file)) {DeleteFileA(temporary);return 0;}return 1;
}
static int ini_integer(const char *path,const char *section,const char *key,int value) {
    char text[32];snprintf(text,sizeof(text),"%d",value);
    return WritePrivateProfileStringA(section,key,text,path)!=0;
}
static int ini_finish(const char *path,const char *temporary,int ok) {
    /* The cache-flush form returns zero even on success. Individual key
       writes above and atomic replacement below carry the failure result. */
    WritePrivateProfileStringA(NULL,NULL,NULL,temporary);
    if(ok)ok=MoveFileExA(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileA(temporary);return ok;
}
static int save_options(void *context,int level,int display,int music) {
    WindowSettings *s=context;char temporary[1240];int ok;
    if(s->path[0]) {
        if(!ini_begin(s->path,temporary))return 0;
        ok=ini_integer(temporary,"Enh","Level",level) && ini_integer(temporary,"Enh","Screen",display) && ini_integer(temporary,"Enh","MidPlayer",music);
        if(!ini_finish(s->path,temporary,ok))return 0;
    }
    return 1;
}
static int save_progress(void *context,const int scores[5],const int modes[5],unsigned weapons) {
    WindowSettings *s=context;char temporary[1240],key[32];unsigned i;int ok=1;
    if(!s->score_path[0])return 1;
    if(!ini_begin(s->score_path,temporary))return 0;
    for(i=0;i<5 && ok;++i) {
        snprintf(key,sizeof(key),"HiScore%u",i+1);ok=ini_integer(temporary,"Score",key,scores[i]);
        if(ok) {snprintf(key,sizeof(key),"ClearMode%u",i+1);ok=ini_integer(temporary,"Score",key,modes[i]);}
    }
    for(i=0;i<11 && ok;++i) {snprintf(key,sizeof(key),"WeaponFlag%u",i);ok=ini_integer(temporary,"Score",key,(weapons>>i)&1);}
    return ini_finish(s->score_path,temporary,ok);
}
static int save_controls(WindowSettings *s) {
    char temporary[1240];unsigned i;int ok;
    if(!s->controls_path[0])return 1;
    if(!ini_begin(s->controls_path,temporary))return 0;
    ok=ini_integer(temporary,"Input","PadDevice",s->controls.device) && ini_integer(temporary,"Input","Deadzone",s->controls.deadzone);
    for(i=0;i<BLAZE_BINDINGS && ok;++i)
        ok=ini_integer(temporary,"Input",binding_keys[i],s->controls.keys[i]) && ini_integer(temporary,"Input",binding_buttons[i],s->controls.buttons[i]);
    return ini_finish(s->controls_path,temporary,ok);
}
static int load_options(WindowSettings *s,BlazeApp *a) {
    char line[160],key[40],raw[80],section[40];FILE *f;unsigned kind;
    const char *paths[]={s->path,s->score_path,s->controls_path};
    controls_defaults(&s->controls);
    for(kind=0;kind<3;++kind)if(paths[kind][0]) {
        section[0]=0;f=fopen(paths[kind],"rb");if(!f && errno!=ENOENT)return 0;
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
    a->high_score=a->ranking_scores[0];
    return 1;
}
/* The application owns the retained framebuffer until after DestroyWindow.
   Repainting must work even while simulation is paused for focus/minimize. */
static void present_frame(HWND window,HDC dc,const BlazeFrame *frame) {
    struct { BITMAPINFOHEADER header; RGBQUAD colors[256]; } bitmap={0};RECT client;
    bitmap.header.biSize=sizeof(BITMAPINFOHEADER);
    bitmap.header.biWidth=BLAZE_WIDTH;bitmap.header.biHeight=-BLAZE_HEIGHT;
    bitmap.header.biPlanes=1;bitmap.header.biBitCount=8;bitmap.header.biCompression=BI_RGB;
    memcpy(bitmap.colors,frame->palette,1024);GetClientRect(window,&client);
    {
    WindowSettings *settings=(WindowSettings *)GetPropA(window,"BlazeSettings");
    int width=client.right,height=client.bottom,x=0,y=0;
    if(settings && settings->mode>=2) {
        PatBlt(dc,0,0,width,height,BLACKNESS);
        if(settings->mode==3) {width=320;height=240;}
        else if((int64_t)width*240>(int64_t)height*320)width=height*320/240;
        else height=width*240/320;
        x=(client.right-width)/2;y=(client.bottom-height)/2;
    }
    StretchDIBits(dc,x,y,width,height,0,0,BLAZE_WIDTH,BLAZE_HEIGHT,
        frame->pixels,(BITMAPINFO *)&bitmap,DIB_RGB_COLORS,SRCCOPY);
    }
}
static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w, LPARAM l) {
    const BlazeFrame *frame=(const BlazeFrame *)GetWindowLongPtrA(window,GWLP_USERDATA);
    if(message==WM_NCCREATE) {
        const CREATESTRUCTA *create=(const CREATESTRUCTA *)l;
        SetWindowLongPtrA(window,GWLP_USERDATA,(LONG_PTR)create->lpCreateParams);
    }
    if(message==WM_PAINT && frame) {
        PAINTSTRUCT paint;HDC dc=BeginPaint(window,&paint);
        present_frame(window,dc,frame);EndPaint(window,&paint);return 0;
    }
    if(message==WM_PRINTCLIENT && frame) {present_frame(window,(HDC)w,frame);return 0;}
    if(message==WM_SIZE) {InvalidateRect(window,NULL,FALSE);return 0;}
    if(message==WM_ERASEBKGND && frame)return 1;
    if (message==WM_CLOSE) { DestroyWindow(window); return 0; }
    if (message==WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(window,message,w,l);
}
static HWND create_game_window(HINSTANCE instance,BlazeFrame *frame) {
    RECT bounds={0,0,640,480};HWND window;
    HICON icon=LoadIconA(instance,MAKEINTRESOURCEA(BLAZE_ICON));
    HICON small=(HICON)LoadImageA(instance,MAKEINTRESOURCEA(BLAZE_ICON),IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);
    if(!icon || !small)return NULL;
    AdjustWindowRect(&bounds,WS_OVERLAPPEDWINDOW,FALSE);
    /* Show without stealing focus; an ordinary click can still activate it. */
    window=CreateWindowExA(0,"BlazeNative","BLAZE",
        WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,
        NULL,NULL,instance,frame);
    if(window) {
        /* LR_SHARED/LoadIcon handles belong to the resource loader. */
        SendMessageA(window,WM_SETICON,ICON_BIG,(LPARAM)icon);
        SendMessageA(window,WM_SETICON,ICON_SMALL,(LPARAM)small);
    }
    return window;
}
static int frame_dump(const char *path,const BlazeFrame *frame) {
    FILE *f=fopen(path,"wb"); int y,x;
    if (!f) return 0;
    fprintf(f,"P6\n320 240\n255\n");
    for (y=0;y<BLAZE_HEIGHT;++y) for (x=0;x<BLAZE_WIDTH;++x) {
        const uint8_t *p=frame->palette+(size_t)frame->pixels[y*BLAZE_WIDTH+x]*4;
        fputc(p[2],f); fputc(p[1],f); fputc(p[0],f);
    }
    return fclose(f)==0;
}
int main(int argc,char **argv) {
    char root[1024],rop[1200],executable[1024];
    BlazeApp *app=calloc(1,sizeof(*app));
    BlazeWindowsAudio *audio=NULL;int mute=0,explicit_audio=0;
    WNDCLASSA cls={0}; HWND window=NULL; MSG message;
    unsigned frames=0,limit=0,seed=0; int hidden=0,exit_code=0,i,quit=0,has_seed=0;
    DWORD path_length;FrameClock clock={0};ULONGLONG started=0;
    LARGE_INTEGER frequency,frame_begin,frame_end;
    double work_total=0,work_max=0;unsigned overruns=0;
    LARGE_INTEGER previous_begin={0};double interval_min=1e30,interval_max=0;
    int precise_wait=0;
    const char *dump=NULL,*script_path=NULL,*settings_path=NULL;
    WindowSettings settings={0};BlazePad pad={-1,0};
    uint8_t *script=NULL;size_t script_size=0;
    if (!app) return 1;
    path_length=GetModuleFileNameA(NULL,executable,sizeof(executable));
    if (!path_length || path_length>=sizeof(executable)) { free(app); return 1; }
    if (!strrchr(executable,'\\')) { free(app); return 1; }
    *strrchr(executable,'\\')='\0'; strcpy(root,executable);
    for (i=1;i<argc;++i) {
        if (!strcmp(argv[i],"--assets") && i+1<argc) {
            if (strlen(argv[++i])>=sizeof(root)) { free(app); return 1; }
            strcpy(root,argv[i]);
        } else if (!strcmp(argv[i],"--frames") && i+1<argc) limit=(unsigned)strtoul(argv[++i],NULL,10);
        else if (!strcmp(argv[i],"--hidden")) hidden=1;
        else if(!strcmp(argv[i],"--mute")) {mute=1;explicit_audio=1;}
        else if(!strcmp(argv[i],"--audio")) {mute=0;explicit_audio=1;}
        else if(!strcmp(argv[i],"--settings") && i+1<argc)settings_path=argv[++i];
        else if (!strcmp(argv[i],"--dump-frame") && i+1<argc) dump=argv[++i];
        else if (!strcmp(argv[i],"--input-script") && i+1<argc) script_path=argv[++i];
        else if (!strcmp(argv[i],"--seed") && i+1<argc) {
            char *end;unsigned long long parsed=strtoull(argv[++i],&end,0);
            if(!*argv[i] || *end || parsed>UINT32_MAX) {fprintf(stderr,"Invalid 32-bit seed\n");free(app);return 1;}
            seed=(unsigned)parsed;has_seed=1;
        }
        else { fprintf(stderr,"Unknown/incomplete argument: %s\n",argv[i]); free(app); return 1; }
    }
    if (hidden && !limit) { fprintf(stderr,"--hidden requires bounded --frames\n"); free(app); return 1; }
    if(script_path && (!limit || !blaze_file_read(script_path,&script,&script_size) || script_size%4)) {
        fprintf(stderr,"Input script requires bounded frames and LE32 action masks\n");free(script);free(app);return 1;
    }
    blaze_path(rop,sizeof(rop),executable,"resources/rop.bin");
    if (!blaze_app_open(app,root,rop)) { fprintf(stderr,"%s\n",app->error); exit_code=1; goto done; }
    if(!settings_location(&settings,executable,settings_path,hidden)) {
        fprintf(stderr,"Cannot prepare dat INI paths\n");exit_code=1;goto done;
    }
    if(!load_options(&settings,app)) {fprintf(stderr,"Cannot load user options\n");exit_code=1;goto done;}
    if(!explicit_audio && hidden)mute=1;
    audio=calloc(1,sizeof(*audio));
    if(!audio || !blaze_windows_audio_open(audio,root,mute)) {exit_code=1;goto done;}
    blaze_app_set_media(app,&audio->media);
    if(has_seed)blaze_random_begin(&app->random,seed);
    cls.lpfnWndProc=window_proc; cls.hInstance=GetModuleHandleA(NULL);
    cls.lpszClassName="BlazeNative"; cls.hCursor=LoadCursor(NULL,IDC_ARROW);
    cls.hIcon=LoadIconA(cls.hInstance,MAKEINTRESOURCEA(BLAZE_ICON));
    if (!RegisterClassA(&cls)) { exit_code=1; goto done; }
    window=create_game_window(cls.hInstance,&app->frame);
    if (!window) { exit_code=1; goto done; }
    settings.window=window;
    if(!SetPropA(window,"BlazeSettings",&settings)) {exit_code=1;goto done;}
    /* Launch windowed by default, including automated runs. Full screen requires
       explicit confirmation in Options. */
    if(!apply_display(&settings,app->display<2?app->display:1)) {exit_code=1;goto done;}
    app->display=settings.mode;app->settings_context=&settings;
    app->apply_display=apply_display;app->save_options=save_options;app->save_progress=save_progress;
    if (!hidden) ShowWindow(window,SW_SHOWNOACTIVATE);
    QueryPerformanceFrequency(&frequency);
    precise_wait=timeBeginPeriod(1)==TIMERR_NOERROR;
    started=clock.next=clock_ms(&frequency);clock.period=app->frame_ms;
    while (!quit && (!limit || frames<limit)) {
        HDC dc;
        while (PeekMessageA(&message,NULL,0,0,PM_REMOVE)) {
            if (message.message==WM_QUIT) { quit=1; break; }
            if(!hidden && (message.message==WM_KEYDOWN || message.message==WM_SYSKEYDOWN) && message.wParam==VK_F10) {
                BlazeControls previous=settings.controls;int edited;
                blaze_app_pause(app,4,1);edited=controls_edit(window,&settings.controls,audio);blaze_app_pause(app,4,0);
                if(edited<0 || (edited && !save_controls(&settings))) {
                    settings.controls=previous;MessageBoxA(window,"Cannot save controls. Previous bindings retained.","Blaze controls",MB_OK|MB_ICONERROR);
                }
                pad.active=-1;pad.retry=0;clock.next=clock_ms(&frequency);previous_begin.QuadPart=0;continue;
            }
            TranslateMessage(&message); DispatchMessageA(&message);
        }
        if (quit) break;
        if (!hidden && frames && (IsIconic(window) || GetForegroundWindow()!=window)) {
            blaze_app_pause(app,1,1);blaze_windows_audio_pump(audio);
            dc=GetDC(window);present_frame(window,dc,&app->frame);ReleaseDC(window,dc);
            clock.next=clock_ms(&frequency);previous_begin.QuadPart=0;MsgWaitForMultipleObjects(0,NULL,FALSE,20,QS_ALLINPUT);continue;
        }
        blaze_app_pause(app,1,0);
        {
            DWORD wait=frame_wait(&clock,clock_ms(&frequency));
            if(wait) {MsgWaitForMultipleObjects(0,NULL,FALSE,wait,QS_ALLINPUT);continue;}
        }
        QueryPerformanceCounter(&frame_begin);
        if(previous_begin.QuadPart) {
            double ms=(double)(frame_begin.QuadPart-previous_begin.QuadPart)*1000/frequency.QuadPart;
            if(ms<interval_min)interval_min=ms;if(ms>interval_max)interval_max=ms;
        }
        previous_begin=frame_begin;
        {
            unsigned actions=script && frames<script_size/4?blaze_le32(script+(size_t)frames*4):hidden?0:controls_poll(window,&settings.controls,&pad);
            if (!blaze_app_tick(app,actions)) { exit_code=1; break; }
        }
        dc=GetDC(window);present_frame(window,dc,&app->frame);
        ReleaseDC(window,dc);blaze_windows_audio_pump(audio); ++frames;
        QueryPerformanceCounter(&frame_end);
        {
            double ms=(double)(frame_end.QuadPart-frame_begin.QuadPart)*1000/frequency.QuadPart;
            work_total+=ms;if(ms>work_max)work_max=ms;if(ms>clock.period)++overruns;
        }
        if (app->state==BLAZE_BLOCKED) { fprintf(stderr,"BLOCKED: %s\n",app->error); exit_code=2; break; }
        if (app->state==BLAZE_EXIT) break;
        frame_advance(&clock,clock_ms(&frequency));
    }
    if (dump && !frame_dump(dump,&app->frame)) exit_code=1;
    printf("BLAZE_NATIVE_C: frames=%u screen=%d; shutdown\n",frames,(int)app->state);
    printf("timing period_ms=%u elapsed_ms=%llu scheduled_frames=%u\n",clock.period,
        (unsigned long long)(clock_ms(&frequency)-started),frames);
    printf("frame work average_ms=%.3f max_ms=%.3f over_budget=%u (update and GDI submission; excludes pacing)\n",
        frames?work_total/frames:0,work_max,overruns);
    if(frames>1)printf("frame interval min_ms=%.3f max_ms=%.3f (includes pauses and scene loads)\n",interval_min,interval_max);
    if(app->world.active) {
        unsigned actors=0,shots=0,effects=0,j;
        for(j=0;j<256;++j)actors+=app->world.actors[j][4]!=0;
        for(j=0;j<48;++j)shots+=app->world.player.words[(0xb20+j*0x34+0x10)/4]!=0;
        for(j=0;j<20;++j)effects+=app->world.player.words[(0x1888+j*0x40+0x10)/4]!=0;
        printf("world tick=%u camera=%d enemies=%u live=%u shots=%u attached=%u health=%u score=%u rng=%08x\n",
            app->world.tick,app->world.camera_x,app->world.enemies_created,actors,shots,effects,
            app->world.player.words[0x2ad0/4],app->world.player.words[0x58/4],app->random.state);
    }
done:
    if (exit_code==1 && app->error[0]) fprintf(stderr,"%s\n",app->error);
    if (window && IsWindow(window)) {RemovePropA(window,"BlazeSettings");DestroyWindow(window);}
    blaze_app_close(app);if(audio)blaze_windows_audio_close(audio);
    if(precise_wait)timeEndPeriod(1);
    free(audio); free(app);free(script); return exit_code;
}
