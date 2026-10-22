/* Exercise the real SDL renderer and virtual input without focus or real input. */
#define SDL_MAIN_HANDLED
#define main blaze_sdl_entry
#include "../../platform/sdl.c"
#undef main
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"SDL line %d: %s (%s)\n",__LINE__,#x,SDL_GetError());goto done;}} while(0)
static int rgb(SDL_Surface *s,int x,int y,unsigned r,unsigned g,unsigned b) {
    Uint8 rr,gg,bb,aa;return SDL_ReadSurfacePixel(s,x,y,&rr,&gg,&bb,&aa) && rr==r && gg==g && bb==b;
}
int main(int argc,char **argv) {
    SdlPlatform *p=calloc(1,sizeof(*p));BlazeFrame *frame=calloc(1,sizeof(*frame));
    SDL_Surface *readback=NULL;SDL_JoystickID virtual_id=0;SDL_VirtualJoystickDesc desc;
    bool keys[SDL_SCANCODE_COUNT]={0};BlazeControls c;unsigned i;int result=1;
    BlazeSdlAudio audio={0};
    HWND foreground=GetForegroundWindow();
    SDL_SetMainReady();CHECK(argc==2 && p && frame && SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK));
    CHECK(SDL_CreateWindowAndRenderer("BLAZE",640,480,SDL_WINDOW_HIDDEN,&p->window,&p->renderer));
    CHECK(!strcmp(SDL_GetWindowTitle(p->window),"BLAZE") && GetForegroundWindow()==foreground);
    {HWND hwnd=(HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(p->window),SDL_PROP_WINDOW_WIN32_HWND_POINTER,NULL);
        CHECK(hwnd && GetClassLongPtrA(hwnd,GCLP_HICON));}
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
    CHECK(!poll_input(p) && GetForegroundWindow()==foreground);
    CHECK(SDL_DetachVirtualJoystick(virtual_id));virtual_id=0;CHECK(!SDL_JoystickConnected(p->joystick));
    p->files.controls.device=0;pad_refresh(p);CHECK(!p->joystick);
    {
        BlazeMusicStatus status={0};unsigned n;
        CHECK(blaze_sdl_audio_open(&audio,argv[1],1));
        audio.media.sound(audio.media.context,BLAZE_SHT);blaze_sdl_audio_pump(&audio);
        CHECK(audio.pcm.mixer.voices[BLAZE_SHT][0].position>0);
        audio.media.silence(audio.media.context);CHECK(!audio.pcm.mixer.voices[BLAZE_SHT][0].active);
        audio.media.load(audio.media.context,0,"blz_sg1f.mid",1,0);
        audio.media.load(audio.media.context,1,"blz_sg1f.mid",1,0);
        audio.media.play(audio.media.context,0,1);audio.media.stop(audio.media.context);audio.media.play(audio.media.context,1,0);
        for(n=0;n<1000;++n) {blaze_windows_music_status(audio.music,&status);if(status.completed==status.submitted)break;SDL_Delay(1);}
        if(n==1000 || status.failed || !status.playing || status.active!=1 || !status.valid[0] || !status.valid[1]) {
            blaze_sdl_audio_close(&audio);CHECK(0);
        }
        audio.media.stop(audio.media.context);blaze_sdl_audio_close(&audio);
    }
    puts("SDL renderer colors/scale/centered mode, saved-key mapping, virtual axes/hats/button32 and detached-owner cleanup passed");result=0;
done:
    blaze_sdl_audio_close(&audio);
    if(virtual_id)SDL_DetachVirtualJoystick(virtual_id);
    if(p && p->joystick)SDL_CloseJoystick(p->joystick);
    if(readback)SDL_DestroySurface(readback);
    if(p && p->texture)SDL_DestroyTexture(p->texture);
    if(p && p->renderer)SDL_DestroyRenderer(p->renderer);
    if(p && p->window)SDL_DestroyWindow(p->window);
    SDL_Quit();free(p);free(frame);return result;
}
