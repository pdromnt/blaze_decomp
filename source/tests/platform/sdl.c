/* Exercise the real SDL renderer and virtual input without focus or real input. */
#include <errno.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#define SDL_MAIN_HANDLED
#define main blaze_sdl_entry
#include "../../platform/sdl.c"
#undef main
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"SDL line %d: %s (%s)\n",__LINE__,#x,SDL_GetError());goto done;}} while(0)
static int rgb(SDL_Surface *s,int x,int y,unsigned r,unsigned g,unsigned b) {
    Uint8 rr,gg,bb,aa;return SDL_ReadSurfacePixel(s,x,y,&rr,&gg,&bb,&aa) && rr==r && gg==g && bb==b;
}
static void press(SdlPlatform *p,BlazeApp *a,SDL_Scancode scan) {
    SDL_Event e={0};e.type=SDL_EVENT_KEY_DOWN;e.key.windowID=SDL_GetWindowID(p->window);e.key.scancode=scan;
    platform_event(p,a,&e,0);e.type=SDL_EVENT_KEY_UP;platform_event(p,a,&e,0);
}
int main(int argc,char **argv) {
    SdlPlatform *p=calloc(1,sizeof(*p));BlazeFrame *frame=calloc(1,sizeof(*frame));
    SDL_Surface *readback=NULL;SDL_JoystickID virtual_id=0;SDL_VirtualJoystickDesc desc;
    bool keys[SDL_SCANCODE_COUNT]={0};BlazeControls c;unsigned i;int result=1;
    BlazeSdlAudio audio={0};BlazeApp *app=calloc(1,sizeof(*app));
#ifdef _WIN32
    HWND foreground=GetForegroundWindow();
#define FOREGROUND_UNCHANGED (GetForegroundWindow()==foreground)
#else
#define FOREGROUND_UNCHANGED (!(SDL_GetWindowFlags(p->window)&SDL_WINDOW_INPUT_FOCUS))
#endif
    SDL_SetMainReady();CHECK(SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy"));
    CHECK(argc==3 && p && frame && app && SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK));
    CHECK(SDL_CreateWindowAndRenderer("BLAZE",640,480,SDL_WINDOW_HIDDEN,&p->window,&p->renderer));
    CHECK(!strcmp(SDL_GetWindowTitle(p->window),"BLAZE") && FOREGROUND_UNCHANGED);
    #ifdef _WIN32
    {HWND hwnd=(HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(p->window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,NULL);
        CHECK(hwnd && GetClassLongPtrA(hwnd,GCLP_HICON));}
    #endif
    p->texture=SDL_CreateTexture(p->renderer,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STREAMING,320,240);
    CHECK(p->texture && SDL_SetTextureScaleMode(p->texture,SDL_SCALEMODE_NEAREST));
    frame->palette[4]=0x33;frame->palette[5]=0x66;frame->palette[6]=0xcc;
    frame->palette[8]=0x44;frame->palette[9]=0x22;frame->palette[10]=0x11;
    memset(frame->pixels,1,sizeof(frame->pixels));
    for(i=0;i<240;++i)memset(frame->pixels+i*320+160,2,160);
    CHECK(render_frame(p,frame));readback=SDL_RenderReadPixels(p->renderer,NULL);CHECK(readback);
    CHECK(rgb(readback,10,10,0xcc,0x66,0x33) && rgb(readback,630,470,0x11,0x22,0x44));
    SDL_DestroySurface(readback);readback=NULL;
    p->mode=3;CHECK(render_frame(p,frame));readback=SDL_RenderReadPixels(p->renderer,NULL);CHECK(readback);
    CHECK(rgb(readback,10,10,0,0,0) && rgb(readback,170,130,0xcc,0x66,0x33) && rgb(readback,470,350,0x11,0x22,0x44));
    SDL_DestroySurface(readback);readback=NULL;
    controls_defaults(&c);
    for(i=0;i<BLAZE_BINDINGS;++i) {
        memset(keys,0,sizeof(keys));CHECK(key_scan(c.keys[i])!=SDL_SCANCODE_UNKNOWN);
        keys[key_scan(c.keys[i])]=true;CHECK(keyboard_actions(&c,keys)==binding_actions[i]);
    }
    controls_key(&c,"KeyFire",'A');memset(keys,0,sizeof(keys));keys[SDL_SCANCODE_A]=true;
    CHECK(keyboard_actions(&c,keys)==BLAZE_CONFIRM);
    /* SDL suppresses non-centering joystick updates without window focus.
       Allow synthetic background updates only in this test; the game keeps
       them disabled and poll_input still rejects our hidden window below. */
    CHECK(SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1"));
    SDL_INIT_INTERFACE(&desc);desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=2;desc.nbuttons=32;desc.nhats=1;desc.name="Blaze owned fixture";
    virtual_id=SDL_AttachVirtualJoystick(&desc);CHECK(virtual_id);p->joystick=SDL_OpenJoystick(virtual_id);CHECK(p->joystick);
    CHECK(SDL_SetJoystickVirtualAxis(p->joystick,0,0) && SDL_SetJoystickVirtualAxis(p->joystick,1,0));
    SDL_UpdateJoysticks();CHECK(!joystick_actions(&c,p->joystick));
    CHECK(SDL_SetJoystickVirtualAxis(p->joystick,0,-8192));SDL_UpdateJoysticks();CHECK(joystick_actions(&c,p->joystick)==BLAZE_LEFT);
    CHECK(SDL_SetJoystickVirtualAxis(p->joystick,0,0) && SDL_SetJoystickVirtualHat(p->joystick,0,SDL_HAT_RIGHTUP));
    SDL_UpdateJoysticks();CHECK(joystick_actions(&c,p->joystick)==(BLAZE_UP|BLAZE_RIGHT));
    controls_key(&c,"ButtonFire",32);CHECK(SDL_SetJoystickVirtualHat(p->joystick,0,SDL_HAT_CENTERED) && SDL_SetJoystickVirtualButton(p->joystick,31,true));
    SDL_UpdateJoysticks();CHECK(joystick_actions(&c,p->joystick)==BLAZE_CONFIRM);
    CHECK(!poll_input(p) && FOREGROUND_UNCHANGED);
    {
        SDL_Event event={0};BlazeSettings reload={0};char rop[1200],preview[1200],saved_path[1200];unsigned tick,n;
        CHECK(blaze_path(rop,sizeof(rop),SDL_GetBasePath(),"resources/rop.bin") && blaze_app_open(app,argv[1],rop,NULL));
        CHECK(blaze_settings_paths(&p->files,argv[2]));controls_defaults(&p->files.controls);p->focused=1;
        remove(p->files.controls_path);tick=app->tick;
        press(p,app,SDL_SCANCODE_F10);CHECK(p->editor.active && app->pause_flags==4);
        for(n=0;n<6;++n)press(p,app,SDL_SCANCODE_DOWN);
        press(p,app,SDL_SCANCODE_RETURN);CHECK(p->editor.capture==0);
        press(p,app,SDL_SCANCODE_A);CHECK(p->editor.value.keys[4]=='A' && p->files.controls.keys[4]=='Z');
        CHECK(blaze_app_tick(app,BLAZE_CONFIRM) && app->tick==tick);
        press(p,app,SDL_SCANCODE_ESCAPE);CHECK(!p->editor.active && !app->pause_flags && p->files.controls.keys[4]=='Z');
        {FILE *f=fopen(p->files.controls_path,"rb");if(f)fclose(f);CHECK(!f);}
        press(p,app,SDL_SCANCODE_F10);
        press(p,app,SDL_SCANCODE_DOWN);press(p,app,SDL_SCANCODE_RIGHT);CHECK(p->editor.value.deadzone==30);
        for(n=0;n<5;++n)press(p,app,SDL_SCANCODE_DOWN);
        press(p,app,SDL_SCANCODE_RETURN);press(p,app,SDL_SCANCODE_BACKSPACE);CHECK(!p->editor.value.keys[4]);
        press(p,app,SDL_SCANCODE_RETURN);press(p,app,SDL_SCANCODE_F10);CHECK(p->editor.active && !p->editor.value.keys[4]);
        press(p,app,SDL_SCANCODE_RETURN);press(p,app,SDL_SCANCODE_RCTRL);CHECK(p->editor.value.keys[4]==0x11);
        press(p,app,SDL_SCANCODE_RETURN);press(p,app,SDL_SCANCODE_A);
        press(p,app,SDL_SCANCODE_RIGHT);press(p,app,SDL_SCANCODE_RETURN);CHECK(p->editor.capture==1);
        event.type=SDL_EVENT_JOYSTICK_BUTTON_DOWN;event.jbutton.which=virtual_id;event.jbutton.button=31;
        CHECK(!platform_event(p,app,&event,0) && p->editor.value.buttons[4]==32 && p->editor.capture==-1);
        press(p,app,SDL_SCANCODE_RETURN);CHECK(p->editor.capture==1);
        event.type=SDL_EVENT_WINDOW_FOCUS_LOST;event.window.windowID=SDL_GetWindowID(p->window);
        CHECK(!platform_event(p,app,&event,0) && p->editor.capture==-1 && !p->focused);
        press(p,app,SDL_SCANCODE_F10);CHECK(p->editor.active);
        event.type=SDL_EVENT_WINDOW_FOCUS_GAINED;CHECK(!platform_event(p,app,&event,0));
        event.type=SDL_EVENT_KEY_UP;event.key.windowID=SDL_GetWindowID(p->window);CHECK(!platform_event(p,app,&event,0));
        CHECK(render_frame(p,frame) && blaze_sdl_controls_draw(&p->editor,p->renderer));
        readback=SDL_RenderReadPixels(p->renderer,NULL);CHECK(readback && rgb(readback,2,2,8,12,20));
        CHECK(blaze_path(preview,sizeof(preview),argv[2],"editor.bmp") && SDL_SaveBMP(readback,preview));
        SDL_DestroySurface(readback);readback=NULL;
        {float x,y;CHECK(SDL_GetRenderScale(p->renderer,&x,&y) && x==1 && y==1);}
        for(n=0;n<6;++n)press(p,app,SDL_SCANCODE_DOWN);CHECK(p->editor.row==12);
        strcpy(saved_path,p->files.controls_path);
        CHECK(blaze_path(p->files.controls_path,sizeof(p->files.controls_path),argv[2],"missing/BlzCtrl.ini"));
        press(p,app,SDL_SCANCODE_RETURN);CHECK(p->editor.active && p->editor.failed && p->files.controls.keys[4]=='Z');
        strcpy(p->files.controls_path,saved_path);press(p,app,SDL_SCANCODE_RETURN);
        CHECK(!p->editor.active && !app->pause_flags && p->files.controls.keys[4]=='A' && p->files.controls.buttons[4]==32);
        CHECK(blaze_settings_paths(&reload,argv[2]) && blaze_settings_load(&reload,app));
        CHECK(reload.controls.keys[4]=='A' && reload.controls.buttons[4]==32 && reload.controls.deadzone==30);
        pad_refresh(p);CHECK(p->joystick && p->release_inputs);
        /* Reopening the virtual driver resets its samples; inject a physical
           button still held as the newly applied controls resume. */
        CHECK(SDL_SetJoystickVirtualButton(p->joystick,31,true));SDL_UpdateJoysticks();
        CHECK(!resume_actions(p,joystick_actions(&p->files.controls,p->joystick)) && p->release_inputs);
        CHECK(SDL_SetJoystickVirtualButton(p->joystick,31,false));SDL_UpdateJoysticks();
        CHECK(!resume_actions(p,joystick_actions(&p->files.controls,p->joystick)) && !p->release_inputs);
        CHECK(SDL_SetJoystickVirtualButton(p->joystick,31,true));SDL_UpdateJoysticks();
        CHECK(resume_actions(p,joystick_actions(&p->files.controls,p->joystick))==BLAZE_CONFIRM);
        blaze_app_pause(app,2,1);press(p,app,SDL_SCANCODE_F10);CHECK(app->pause_flags==6);
        for(n=0;n<11;++n)press(p,app,SDL_SCANCODE_DOWN);press(p,app,SDL_SCANCODE_RETURN);
        CHECK(p->editor.value.keys[4]=='Z' && p->editor.value.deadzone==25 && p->files.controls.keys[4]=='A');
        press(p,app,SDL_SCANCODE_ESCAPE);CHECK(app->pause_flags==2);blaze_app_pause(app,2,0);
        for(n=0;n<12;++n) {press(p,app,SDL_SCANCODE_F10);CHECK(p->editor.active);press(p,app,SDL_SCANCODE_ESCAPE);CHECK(!p->editor.pad);}
        CHECK(p->joystick && SDL_GetJoystickFromID(virtual_id)==p->joystick);
        /* Pointer coordinates use the same output transform as the pixel menu.
           Synthetic events never move or capture the host pointer. */
        press(p,app,SDL_SCANCODE_F10);event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;event.button.windowID=SDL_GetWindowID(p->window);
        event.button.button=SDL_BUTTON_LEFT;event.button.x=280;event.button.y=238;
        CHECK(!platform_event(p,app,&event,0) && p->editor.row==6 && !p->editor.column && p->editor.capture==0);
        press(p,app,SDL_SCANCODE_B);CHECK(p->editor.value.keys[4]=='B');
        event.button.x=500;event.button.y=378;CHECK(!platform_event(p,app,&event,0) && !p->editor.active && p->files.controls.keys[4]=='A');
        press(p,app,SDL_SCANCODE_F10);event.type=SDL_EVENT_QUIT;CHECK(platform_event(p,app,&event,0));
        blaze_sdl_controls_close(&p->editor);blaze_app_pause(app,4,0);
        press(p,app,SDL_SCANCODE_F10);for(n=0;n<6;++n)press(p,app,SDL_SCANCODE_DOWN);
        press(p,app,SDL_SCANCODE_RIGHT);press(p,app,SDL_SCANCODE_RETURN);CHECK(p->editor.capture==1 && p->editor.pad);
        event.type=SDL_EVENT_JOYSTICK_REMOVED;event.jdevice.which=virtual_id;
        CHECK(SDL_DetachVirtualJoystick(virtual_id));virtual_id=0;
        CHECK(!platform_event(p,app,&event,0) && p->editor.capture==-1 && !p->editor.pad && p->editor.active);
        press(p,app,SDL_SCANCODE_ESCAPE);
        CHECK(FOREGROUND_UNCHANGED);remove(p->files.controls_path);
    }
    CHECK(!SDL_JoystickConnected(p->joystick));
    p->files.controls.device=0;pad_refresh(p);CHECK(!p->joystick);
    {
        BlazeSettings user={0},disabled={0};char directory[1200],file[1200],actual[1200],requested[1200],rop[1200];
        FILE *f;uint8_t *bytes=NULL;size_t size=0;
        CHECK(blaze_path(directory,sizeof(directory),argv[2],"paths-\xc3\xa7-\xe6\xb8\xb8"));
        CHECK(blaze_sdl_paths(&user,argv[1],directory,1,0));
        CHECK(blaze_sdl_paths(&disabled,argv[1],"none",0,0) && !disabled.path[0]);
        f=blaze_fopen(user.path,"wb");CHECK(f);
        CHECK(fputs("[Enh]\nTime=41\nWeaponName1=PORTABLE\nWeaponPower1=7\n[MIDI]\nStart1=blz_sg1f.mid\n",f)>=0 && !fclose(f));
        /* Seeding again must preserve edits, including fields not written by
           the options menu. Startup must use this full user configuration. */
        CHECK(blaze_sdl_paths(&user,argv[1],directory,1,0));
        blaze_app_close(app);CHECK(blaze_path(rop,sizeof(rop),SDL_GetBasePath(),"resources/rop.bin"));
        CHECK(blaze_app_open(app,argv[1],rop,directory));
        CHECK(app->frame_ms==41 && !strcmp(app->loadout.weapons[0].name,"PORTABLE") && app->loadout.weapons[0].power==7);
        CHECK(!strcmp(app->tracks[0][0].file,"blz_sg1f.mid"));
        controls_defaults(&user.controls);user.controls.keys[4]='B';CHECK(blaze_settings_controls(&user));
        CHECK(blaze_settings_load(&user,app) && user.controls.keys[4]=='B');
        CHECK(blaze_settings_options(&user,2,1,1) && blaze_settings_load(&user,app) && app->level==2);
        CHECK(blaze_path(file,sizeof(file),directory,"CaseDir") && SDL_CreateDirectory(file));
        CHECK(blaze_path(actual,sizeof(actual),file,"MiXeD.bin"));f=blaze_fopen(actual,"wb");CHECK(f);
        CHECK(fwrite("case-fixture",1,12,f)==12 && !fclose(f));
        CHECK(blaze_path(requested,sizeof(requested),directory,"casedir\\mixed.BIN"));
        CHECK(blaze_file_read(requested,&bytes,&size) && size==12 && !memcmp(bytes,"case-fixture",12));free(bytes);bytes=NULL;
#ifndef _WIN32
        CHECK(blaze_path(requested,sizeof(requested),file,"mixed.BIN"));f=blaze_fopen(requested,"wbx");
        if(f) {
            CHECK(!fclose(f));
            CHECK(blaze_path(requested,sizeof(requested),file,"MIXED.BIN"));CHECK(!blaze_asset_open(requested));
            CHECK(blaze_path(requested,sizeof(requested),file,"mixed.BIN"));CHECK(!blaze_file_remove(requested));
        } else {
            /* A case-insensitive volume cannot contain both names. Exclusive
               creation must fail without truncating the existing fixture. */
            CHECK(errno==EEXIST);
            CHECK(blaze_file_read(requested,&bytes,&size) && size==12 && !memcmp(bytes,"case-fixture",12));free(bytes);bytes=NULL;
        }
#endif
        CHECK(!blaze_file_remove(actual));
        CHECK(!blaze_file_remove(user.path) && !blaze_file_remove(user.score_path) && !blaze_file_remove(user.controls_path));
    }
    {
        BlazeMusicStatus status={0};unsigned n;
        CHECK(blaze_sdl_audio_open(&audio,argv[1],1));
        audio.media.sound(audio.media.context,BLAZE_SHT);blaze_sdl_audio_pump(&audio);
        CHECK(audio.pcm.mixer.voices[BLAZE_SHT][0].position>0);
        audio.media.silence(audio.media.context);CHECK(!audio.pcm.mixer.voices[BLAZE_SHT][0].active);
        audio.media.load(audio.media.context,0,"blz_sg1f.mid",1,0);
        audio.media.load(audio.media.context,1,"blz_sg1f.mid",1,0);
        audio.media.play(audio.media.context,0,1);audio.media.stop(audio.media.context);audio.media.play(audio.media.context,1,0);
        for(n=0;n<1000;++n) {blaze_sdl_music_status(audio.music,&status);if(status.completed==status.submitted)break;SDL_Delay(1);}
        if(n==1000 || status.failed || !status.playing || status.active!=1 || !status.valid[0] || !status.valid[1]) {
            blaze_sdl_audio_close(&audio);CHECK(0);
        }
        audio.media.stop(audio.media.context);blaze_sdl_audio_close(&audio);
    }
    {
        uint64_t submitted;unsigned tick=app->tick;
        CHECK(blaze_sdl_audio_open(&audio,argv[1],0) && !strcmp(SDL_GetCurrentAudioDriver(),"dummy"));
        audio.media.load(audio.media.context,0,"blz_sg1f.mid",1,0);audio.media.play(audio.media.context,0,1);
        press(p,app,SDL_SCANCODE_F10);CHECK(p->editor.active && app->pause_flags==4);
        audio.media.sound(audio.media.context,BLAZE_SHT);submitted=blaze_sdl_pcm_submitted(&audio.pcm);
        CHECK(present(p,&app->frame));SDL_Delay(400);
        CHECK(blaze_sdl_pcm_submitted(&audio.pcm)>submitted+6000 && app->tick==tick && p->editor.active);
        {BlazeMusicStatus status;blaze_sdl_music_status(audio.music,&status);CHECK(status.playing && !status.failed);}
        press(p,app,SDL_SCANCODE_ESCAPE);CHECK(!app->pause_flags);blaze_sdl_audio_close(&audio);
    }
    puts("SDL rendering/input and F10 capture/cancel/defaults/save/failure/focus/pause/close lifecycle passed");result=0;
done:
    blaze_sdl_audio_close(&audio);
    if(app)blaze_app_close(app);
    if(p)blaze_sdl_controls_close(&p->editor);
    if(virtual_id)SDL_DetachVirtualJoystick(virtual_id);
    if(p && p->joystick)SDL_CloseJoystick(p->joystick);
    if(readback)SDL_DestroySurface(readback);
    if(p && p->texture)SDL_DestroyTexture(p->texture);
    if(p && p->renderer)SDL_DestroyRenderer(p->renderer);
    if(p && p->window)SDL_DestroyWindow(p->window);
    SDL_Quit();free(p);free(frame);free(app);return result;
}
