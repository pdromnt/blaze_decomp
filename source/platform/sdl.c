/* SDL presentation/input/PCM/timing candidate. Windows MIDI and F10 editor remain
   explicit native dependencies for this first connected application milestone. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sdl_audio.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "settings.h"
#include "windows_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    SDL_Window *window;SDL_Renderer *renderer;SDL_Texture *texture;
    BlazeSettings files;SDL_Joystick *joystick;
    int mode;
} SdlPlatform;
/* Fixed storage format, independent of SDL physical scancode numbering. */
static SDL_Scancode key_scan(int key) {
    if(key>='A' && key<='Z')return SDL_GetScancodeFromKey((SDL_Keycode)(SDLK_A+key-'A'),NULL);
    if(key>='1' && key<='9')return (SDL_Scancode)(SDL_SCANCODE_1+key-'1');
    if(key>=0x70 && key<=0x87)return (SDL_Scancode)(key<0x7c?SDL_SCANCODE_F1+key-0x70:SDL_SCANCODE_F13+key-0x7c);
    if(key>=0x60 && key<=0x69)return key==0x60?SDL_SCANCODE_KP_0:(SDL_Scancode)(SDL_SCANCODE_KP_1+key-0x61);
    switch(key) {
    case '0':return SDL_SCANCODE_0;case 8:return SDL_SCANCODE_BACKSPACE;case 9:return SDL_SCANCODE_TAB;
    case 13:return SDL_SCANCODE_RETURN;case 0x10:return SDL_SCANCODE_LSHIFT;case 0x11:return SDL_SCANCODE_LCTRL;
    case 0x12:return SDL_SCANCODE_LALT;case 0x13:return SDL_SCANCODE_PAUSE;case 0x14:return SDL_SCANCODE_CAPSLOCK;
    case 0x1b:return SDL_SCANCODE_ESCAPE;case 0x20:return SDL_SCANCODE_SPACE;
    case 0x21:return SDL_SCANCODE_PAGEUP;case 0x22:return SDL_SCANCODE_PAGEDOWN;case 0x23:return SDL_SCANCODE_END;
    case 0x24:return SDL_SCANCODE_HOME;case 0x25:return SDL_SCANCODE_LEFT;case 0x26:return SDL_SCANCODE_UP;
    case 0x27:return SDL_SCANCODE_RIGHT;case 0x28:return SDL_SCANCODE_DOWN;case 0x2c:return SDL_SCANCODE_PRINTSCREEN;
    case 0x2d:return SDL_SCANCODE_INSERT;case 0x2e:return SDL_SCANCODE_DELETE;
    case 0x5b:return SDL_SCANCODE_LGUI;case 0x5c:return SDL_SCANCODE_RGUI;case 0x5d:return SDL_SCANCODE_APPLICATION;
    case 0x6a:return SDL_SCANCODE_KP_MULTIPLY;case 0x6b:return SDL_SCANCODE_KP_PLUS;
    case 0x6d:return SDL_SCANCODE_KP_MINUS;case 0x6e:return SDL_SCANCODE_KP_PERIOD;case 0x6f:return SDL_SCANCODE_KP_DIVIDE;
    case 0x90:return SDL_SCANCODE_NUMLOCKCLEAR;case 0x91:return SDL_SCANCODE_SCROLLLOCK;
    case 0xa0:return SDL_SCANCODE_LSHIFT;case 0xa1:return SDL_SCANCODE_RSHIFT;
    case 0xa2:return SDL_SCANCODE_LCTRL;case 0xa3:return SDL_SCANCODE_RCTRL;
    case 0xa4:return SDL_SCANCODE_LALT;case 0xa5:return SDL_SCANCODE_RALT;
    case 0xba:return SDL_SCANCODE_SEMICOLON;case 0xbb:return SDL_SCANCODE_EQUALS;case 0xbc:return SDL_SCANCODE_COMMA;
    case 0xbd:return SDL_SCANCODE_MINUS;case 0xbe:return SDL_SCANCODE_PERIOD;case 0xbf:return SDL_SCANCODE_SLASH;
    case 0xc0:return SDL_SCANCODE_GRAVE;case 0xdb:return SDL_SCANCODE_LEFTBRACKET;case 0xdc:return SDL_SCANCODE_BACKSLASH;
    case 0xdd:return SDL_SCANCODE_RIGHTBRACKET;case 0xde:return SDL_SCANCODE_APOSTROPHE;
    default:return SDL_SCANCODE_UNKNOWN;
    }
}
static int key_down(const bool *keys,int key) {
    SDL_Scancode scan=key_scan(key);
    if(key==0x10)return keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    if(key==0x11)return keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    if(key==0x12)return keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT];
    if(key==13)return keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_KP_ENTER];
    return scan!=SDL_SCANCODE_UNKNOWN && keys[scan];
}
static int bindings_supported(const BlazeControls *c) {
    unsigned i;for(i=0;i<BLAZE_BINDINGS;++i)if(c->keys[i] && key_scan(c->keys[i])==SDL_SCANCODE_UNKNOWN) {
        fprintf(stderr,"SDL input: saved key ID %d has no mapping yet (%s)\n",c->keys[i],binding_names[i]);return 0;
    }
    return 1;
}
static void pad_refresh(SdlPlatform *p) {
    SDL_JoystickID *ids;int count=0,index;
    if(p->joystick && !SDL_JoystickConnected(p->joystick)) {SDL_CloseJoystick(p->joystick);p->joystick=NULL;}
    if(p->files.controls.device==0) {if(p->joystick)SDL_CloseJoystick(p->joystick);p->joystick=NULL;return;}
    if(p->joystick)return;
    ids=SDL_GetJoysticks(&count);index=p->files.controls.device<0?0:p->files.controls.device-1;
    if(ids && index<count)p->joystick=SDL_OpenJoystick(ids[index]);
    SDL_free(ids);
}
static unsigned keyboard_actions(const BlazeControls *c,const bool *keys) {
    unsigned i,mask=0;for(i=0;i<BLAZE_BINDINGS;++i)if(c->keys[i] && key_down(keys,c->keys[i]))mask|=binding_actions[i];
    return mask;
}
static unsigned joystick_actions(const BlazeControls *c,SDL_Joystick *joystick) {
    unsigned i,mask=0;int axes,buttons,x,y,zone;
    if(!joystick)return 0;
    axes=SDL_GetNumJoystickAxes(joystick);buttons=SDL_GetNumJoystickButtons(joystick);
    x=axes>0?SDL_GetJoystickAxis(joystick,0):0;y=axes>1?SDL_GetJoystickAxis(joystick,1):0;
    zone=32767*c->deadzone/100;
    if(x < -zone)mask|=BLAZE_LEFT;if(x>zone)mask|=BLAZE_RIGHT;
    if(y < -zone)mask|=BLAZE_UP;if(y>zone)mask|=BLAZE_DOWN;
    if(SDL_GetNumJoystickHats(joystick)>0) {
        Uint8 hat=SDL_GetJoystickHat(joystick,0);
        if(hat&SDL_HAT_UP)mask|=BLAZE_UP;if(hat&SDL_HAT_DOWN)mask|=BLAZE_DOWN;
        if(hat&SDL_HAT_LEFT)mask|=BLAZE_LEFT;if(hat&SDL_HAT_RIGHT)mask|=BLAZE_RIGHT;
    }
    for(i=0;i<BLAZE_BINDINGS;++i)if(c->buttons[i]>0 && c->buttons[i]<=buttons && SDL_GetJoystickButton(joystick,c->buttons[i]-1))mask|=binding_actions[i];
    return mask;
}
static unsigned poll_input(SdlPlatform *p) {
    if(!(SDL_GetWindowFlags(p->window)&SDL_WINDOW_INPUT_FOCUS))return 0;
    pad_refresh(p);
    return keyboard_actions(&p->files.controls,SDL_GetKeyboardState(NULL))|joystick_actions(&p->files.controls,p->joystick);
}
static int display(void *context,int mode) {
    SdlPlatform *p=context;
    if(mode<0 || mode>4)return 0;
    if(!SDL_SetWindowFullscreen(p->window,mode>=2))return 0;
    if(mode<2 && !SDL_SetWindowSize(p->window,320*(mode+1),240*(mode+1)))return 0;
    p->mode=mode;return 1;
}
static int options(void *context,int level,int screen,int music) {return blaze_settings_options(&((SdlPlatform *)context)->files,level,screen,music);}
static int progress(void *context,const int scores[5],const int modes[5],unsigned weapons) {return blaze_settings_progress(&((SdlPlatform *)context)->files,scores,modes,weapons);}
static int render_frame(SdlPlatform *p,const BlazeFrame *frame) {
    void *locked;int pitch,y,x,width,height;SDL_FRect dest;
    if(!SDL_LockTexture(p->texture,NULL,&locked,&pitch))return 0;
    for(y=0;y<BLAZE_HEIGHT;++y)for(x=0;x<BLAZE_WIDTH;++x) {
        const uint8_t *color=frame->palette+frame->pixels[y*BLAZE_WIDTH+x]*4;
        uint8_t *pixel=(uint8_t *)locked+y*pitch+x*4;
        pixel[0]=color[2];pixel[1]=color[1];pixel[2]=color[0];pixel[3]=255;
    }
    SDL_UnlockTexture(p->texture);
    if(!SDL_GetRenderOutputSize(p->renderer,&width,&height))return 0;
    dest.w=(float)width;dest.h=(float)height;
    if(p->mode==3) {dest.w=320;dest.h=240;}
    else if((int64_t)width*240>(int64_t)height*320)dest.w=(float)height*320/240;
    else dest.h=(float)width*240/320;
    dest.x=((float)width-dest.w)/2;dest.y=((float)height-dest.h)/2;
    return SDL_SetRenderDrawColor(p->renderer,0,0,0,255) && SDL_RenderClear(p->renderer) &&
        SDL_RenderTexture(p->renderer,p->texture,NULL,&dest);
}
static int present(SdlPlatform *p,const BlazeFrame *frame) {
    return render_frame(p,frame) && SDL_RenderPresent(p->renderer);
}
static int dump_frame(const char *path,const BlazeFrame *frame) {
    FILE *f=fopen(path,"wb");unsigned i;if(!f)return 0;
    if(fprintf(f,"P6\n320 240\n255\n")<0) {fclose(f);return 0;}
    for(i=0;i<320*240;++i) {
        const uint8_t *c=frame->palette+frame->pixels[i]*4;uint8_t rgb[]={c[2],c[1],c[0]};
        if(fwrite(rgb,1,3,f)!=3) {fclose(f);return 0;}
    }
    return fclose(f)==0;
}
int main(int argc,char **argv) {
    SdlPlatform *p=calloc(1,sizeof(*p));BlazeApp *a=calloc(1,sizeof(*a));BlazeSdlAudio *audio=calloc(1,sizeof(*audio));
    char rop[1200],folder[1200];const char *root=NULL,*requested=NULL,*dump=NULL,*input=NULL;
    unsigned frames=0,limit=0,seed=0;int hidden=0,muted=0,explicit_audio=0,has_seed=0,quit=0,result=1,i,initialized=0;
    uint8_t *script=NULL;size_t script_size=0;Uint64 next=0,started=0,previous=0;unsigned period=40,overruns=0;
    double work_total=0,work_max=0,interval_min=1e30,interval_max=0;
    if(!p || !a || !audio)goto done;
    for(i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--hidden"))hidden=1;
        else if(!strcmp(argv[i],"--mute")) {muted=1;explicit_audio=1;}
        else if(!strcmp(argv[i],"--audio")) {muted=0;explicit_audio=1;}
        else if(!strcmp(argv[i],"--assets") && i+1<argc)root=argv[++i];
        else if(!strcmp(argv[i],"--settings") && i+1<argc)requested=argv[++i];
        else if(!strcmp(argv[i],"--dump-frame") && i+1<argc)dump=argv[++i];
        else if(!strcmp(argv[i],"--input-script") && i+1<argc)input=argv[++i];
        else if(!strcmp(argv[i],"--frames") && i+1<argc)limit=(unsigned)strtoul(argv[++i],NULL,10);
        else if(!strcmp(argv[i],"--seed") && i+1<argc) {
            char *end;unsigned long long value=strtoull(argv[++i],&end,0);
            if(!*argv[i] || *end || value>UINT32_MAX)goto done;seed=(unsigned)value;has_seed=1;
        } else {fprintf(stderr,"Unknown/incomplete argument: %s\n",argv[i]);goto done;}
    }
    if(hidden && !limit) {fprintf(stderr,"--hidden requires bounded --frames\n");goto done;}
    if(input && (!limit || !blaze_file_read(input,&script,&script_size) || script_size%4)) {fprintf(stderr,"Invalid bounded LE32 input script\n");goto done;}
    if(!explicit_audio && hidden)muted=1;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"0");
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE,"0");
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN,"0");
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK))goto done;initialized=1;
    if(!SDL_GetBasePath())goto done;
    if(!root)root=SDL_GetBasePath();
    if(!blaze_path(rop,sizeof(rop),SDL_GetBasePath(),"resources/rop.bin") || !blaze_app_open(a,root,rop))goto done;
    if(requested && !strcmp(requested,"none"))requested=NULL;
    else if(!requested && !hidden) {
        if(!blaze_path(folder,sizeof(folder),SDL_GetBasePath(),"dat"))goto done;requested=folder;
    }
    if(!blaze_settings_paths(&p->files,requested) || !blaze_settings_load(&p->files,a) || !bindings_supported(&p->files.controls))goto done;
    if(!blaze_sdl_audio_open(audio,root,muted))goto done;
    blaze_app_set_media(a,&audio->media);if(has_seed)blaze_random_begin(&a->random,seed);
    if(!SDL_CreateWindowAndRenderer("BLAZE",640,480,SDL_WINDOW_HIDDEN|SDL_WINDOW_RESIZABLE,&p->window,&p->renderer))goto done;
    p->texture=SDL_CreateTexture(p->renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,320,240);
    if(!p->texture || !SDL_SetTextureScaleMode(p->texture,SDL_SCALEMODE_NEAREST))goto done;
    /* VSync cannot pace the original 25-tick simulation. */
    if(!SDL_SetRenderVSync(p->renderer,0))goto done;
    if(!display(p,a->display<2?a->display:1))goto done;
    a->display=p->mode;a->settings_context=p;a->apply_display=display;a->save_options=options;a->save_progress=progress;
    if(!hidden && !SDL_ShowWindow(p->window))goto done;
    started=next=SDL_GetTicksNS();period=a->frame_ms;result=0;
    while(!quit && (!limit || frames<limit)) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_EVENT_QUIT)quit=1;
            if(!hidden && event.type==SDL_EVENT_KEY_DOWN && event.key.scancode==SDL_SCANCODE_F10 && !event.key.repeat) {
                HWND hwnd=(HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(p->window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,NULL);
                BlazeControls previous_controls=p->files.controls;int edited;
                blaze_app_pause(a,4,1);edited=hwnd?controls_edit(hwnd,&p->files.controls,blaze_sdl_audio_pump,audio):-1;blaze_app_pause(a,4,0);
                if(edited<0 || (edited && (!bindings_supported(&p->files.controls) || !blaze_settings_controls(&p->files)))) {
                    p->files.controls=previous_controls;SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"BLAZE controls","Cannot save controls. Previous bindings retained.",p->window);
                }
                if(p->joystick)SDL_CloseJoystick(p->joystick);p->joystick=NULL;SDL_ResetKeyboard();next=SDL_GetTicksNS();previous=0;
            }
        }
        if(quit)break;
        if(!hidden && frames && (!(SDL_GetWindowFlags(p->window)&SDL_WINDOW_INPUT_FOCUS) || (SDL_GetWindowFlags(p->window)&SDL_WINDOW_MINIMIZED))) {
            blaze_app_pause(a,1,1);blaze_sdl_audio_pump(audio);
            if(!present(p,&a->frame)) {result=1;break;}
            next=SDL_GetTicksNS();previous=0;SDL_Delay(20);continue;
        }
        blaze_app_pause(a,1,0);
        {Uint64 now=SDL_GetTicksNS();if(now<next) {SDL_DelayPrecise(next-now);continue;}}
        {Uint64 begin=SDL_GetTicksNS();double ms;
            unsigned actions=script && frames<script_size/4?blaze_le32(script+(size_t)frames*4):hidden?0:poll_input(p);
            if(previous) {ms=(double)(begin-previous)/1000000;if(ms<interval_min)interval_min=ms;if(ms>interval_max)interval_max=ms;}
            previous=begin;
            if(!blaze_app_tick(a,actions)) {result=1;break;}
            if(!present(p,&a->frame)) {result=1;break;}
            blaze_sdl_audio_pump(audio);++frames;
            if(blaze_sdl_pcm_failed(&audio->pcm)) {fprintf(stderr,"SDL PCM submission failed\n");result=1;break;}
            ms=(double)(SDL_GetTicksNS()-begin)/1000000;work_total+=ms;if(ms>work_max)work_max=ms;if(ms>period)++overruns;
        }
        if(a->state==BLAZE_BLOCKED) {result=2;break;}if(a->state==BLAZE_EXIT)break;
        next+=(Uint64)period*1000000;{Uint64 now=SDL_GetTicksNS();if(next<now)next=now;}
    }
    if(dump && !dump_frame(dump,&a->frame))result=1;
    printf("BLAZE_NATIVE_C: SDL3 frames=%u screen=%d; shutdown\n",frames,(int)a->state);
    printf("timing period_ms=%u elapsed_ms=%llu scheduled_frames=%u\n",period,(unsigned long long)((SDL_GetTicksNS()-started)/1000000),frames);
    printf("frame work average_ms=%.3f max_ms=%.3f over_budget=%u (update and SDL submission; excludes pacing and separate GPU time)\n",frames?work_total/frames:0,work_max,overruns);
    if(frames>1)printf("frame interval min_ms=%.3f max_ms=%.3f (includes pauses and scene loads)\n",interval_min,interval_max);
    if(a->world.active) {
        unsigned j,actors=0,shots=0,effects=0;
        for(j=0;j<256;++j)actors+=a->world.actors[j][4]!=0;
        for(j=0;j<48;++j)shots+=a->world.player.words[(0xb20+j*0x34+0x10)/4]!=0;
        for(j=0;j<20;++j)effects+=a->world.player.words[(0x1888+j*0x40+0x10)/4]!=0;
        printf("world tick=%u camera=%d enemies=%u live=%u shots=%u attached=%u health=%u score=%u rng=%08x\n",a->world.tick,a->world.camera_x,a->world.enemies_created,actors,shots,effects,a->world.player.words[0x2ad0/4],a->world.player.words[0x58/4],a->random.state);
    }
done:
    if(result)fprintf(stderr,"SDL candidate: %s; %s\n",initialized?SDL_GetError():"startup failed",a?a->error:"");
    if(p && p->joystick)SDL_CloseJoystick(p->joystick);
    if(p && p->texture)SDL_DestroyTexture(p->texture);
    if(p && p->renderer)SDL_DestroyRenderer(p->renderer);
    if(p && p->window)SDL_DestroyWindow(p->window);
    if(a)blaze_app_close(a);if(audio)blaze_sdl_audio_close(audio);
    if(initialized)SDL_Quit();free(script);free(audio);free(a);free(p);return result;
}
