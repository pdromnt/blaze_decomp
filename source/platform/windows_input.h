#ifndef BLAZE_WINDOWS_INPUT_H
#define BLAZE_WINDOWS_INPUT_H
#include <mmsystem.h>
/* Native input configuration, owned by the window backend. Keyboard and pad
   feed the same game action mask; no input is captured or polled unfocused. */
enum { BLAZE_BINDINGS=9 };
static const unsigned binding_actions[BLAZE_BINDINGS]={BLAZE_UP,BLAZE_DOWN,BLAZE_LEFT,BLAZE_RIGHT,BLAZE_CONFIRM,BLAZE_CANCEL,BLAZE_PAUSE,BLAZE_MAIN,BLAZE_QUIT};
static const char *binding_names[BLAZE_BINDINGS]={"Up","Down","Left","Right","Fire / Confirm","Ability / Cancel","Pause","Main menu","Quit"};
static const char *binding_keys[BLAZE_BINDINGS]={"KeyUp","KeyDown","KeyLeft","KeyRight","KeyFire","KeyAbility","KeyPause","KeyMain","KeyQuit"};
static const char *binding_buttons[BLAZE_BINDINGS]={"ButtonUp","ButtonDown","ButtonLeft","ButtonRight","ButtonFire","ButtonAbility","ButtonPause","ButtonMain","ButtonQuit"};
typedef struct {int keys[BLAZE_BINDINGS],buttons[BLAZE_BINDINGS],device,deadzone;} BlazeControls;
typedef struct {int active;ULONGLONG retry;} BlazePad;
static void controls_defaults(BlazeControls *c) {
    static const int keys[]={VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,'Z','X',VK_ESCAPE,VK_F2,VK_F9};
    memset(c,0,sizeof(*c));memcpy(c->keys,keys,sizeof(keys));c->device=-1;c->deadzone=25;
    c->buttons[4]=1;c->buttons[5]=2;c->buttons[6]=8;
}
static void controls_key(BlazeControls *c,const char *key,int value) {
    unsigned i;
    if(!strcmp(key,"PadDevice") && value>=-1 && value<=16)c->device=value;
    if(!strcmp(key,"Deadzone") && value>=0 && value<=95)c->deadzone=value;
    for(i=0;i<BLAZE_BINDINGS;++i) {
        if(!strcmp(key,binding_keys[i]) && value>=0 && value<=255 && value!=VK_F10)c->keys[i]=value;
        if(!strcmp(key,binding_buttons[i]) && value>=0 && value<=32)c->buttons[i]=value;
    }
}
static unsigned controls_keyboard(const BlazeControls *c,const unsigned char down[256]) {
    unsigned i,mask=0;
    for(i=0;i<BLAZE_BINDINGS;++i)if(c->keys[i] && down[c->keys[i]])mask|=binding_actions[i];
    return mask;
}
static unsigned controls_pad(const BlazeControls *c,const JOYCAPSA *caps,const JOYINFOEX *p) {
    unsigned i,mask=0;uint64_t x=p->dwXpos,y=p->dwYpos;
    uint64_t xrange=(uint64_t)caps->wXmax-caps->wXmin,yrange=(uint64_t)caps->wYmax-caps->wYmin;
    uint64_t xm=(uint64_t)caps->wXmin+xrange/2,ym=(uint64_t)caps->wYmin+yrange/2;
    uint64_t dx=xrange*(unsigned)c->deadzone/200,dy=yrange*(unsigned)c->deadzone/200;
    if(xrange) {if(x+dx<xm)mask|=BLAZE_LEFT;else if(x>xm+dx)mask|=BLAZE_RIGHT;}
    if(yrange) {if(y+dy<ym)mask|=BLAZE_UP;else if(y>ym+dy)mask|=BLAZE_DOWN;}
    if((caps->wCaps&JOYCAPS_HASPOV) && p->dwPOV!=JOY_POVCENTERED && p->dwPOV<36000) {
        unsigned sector=(p->dwPOV+2250)/4500%8;
        if(sector==7 || sector<=1)mask|=BLAZE_UP;
        if(sector>=3 && sector<=5)mask|=BLAZE_DOWN;
        if(sector>=1 && sector<=3)mask|=BLAZE_RIGHT;
        if(sector>=5 && sector<=7)mask|=BLAZE_LEFT;
    }
    for(i=0;i<BLAZE_BINDINGS;++i)if(c->buttons[i] && (p->dwButtons&(1u<<(c->buttons[i]-1))))mask|=binding_actions[i];
    return mask;
}
static unsigned controls_poll(HWND window,const BlazeControls *c,BlazePad *pad) {
    unsigned char down[256]={0};unsigned i,mask;ULONGLONG now=GetTickCount64();
    JOYINFOEX position={sizeof(position),JOY_RETURNX|JOY_RETURNY|JOY_RETURNBUTTONS|JOY_RETURNPOV};JOYCAPSA caps;
    if(GetForegroundWindow()!=window)return 0;
    for(i=1;i<256;++i)down[i]=(GetAsyncKeyState((int)i)&0x8000)!=0;
    mask=controls_keyboard(c,down);
    if(!c->device)return mask;
    if(pad->active>=0 && (c->device<0 || pad->active==c->device-1) &&
       joyGetDevCapsA((UINT)pad->active,&caps,sizeof(caps))==JOYERR_NOERROR &&
       joyGetPosEx((UINT)pad->active,&position)==JOYERR_NOERROR)return mask|controls_pad(c,&caps,&position);
    pad->active=-1;
    if(now<pad->retry)return mask;
    pad->retry=now+1000;
    for(i=0;i<joyGetNumDevs() && i<16;++i) {
        if(c->device>0 && i!=(unsigned)c->device-1)continue;
        if(joyGetDevCapsA(i,&caps,sizeof(caps))==JOYERR_NOERROR && joyGetPosEx(i,&position)==JOYERR_NOERROR) {
            pad->active=(int)i;return mask|controls_pad(c,&caps,&position);
        }
    }
    return mask;
}
/* F10 opens an ordinary owned Win32 editor. This is a native enhancement;
   the original game's exposed key-remapping UI has not been established. */
typedef struct {BlazeControls value;HWND keys[9],buttons[9],device,deadzone;int capture,done,saved;} ControlsEditor;
static void controls_label(HWND button,int key) {
    char name[64]="Unbound";
    if(key) {UINT scan=MapVirtualKeyA((UINT)key,MAPVK_VK_TO_VSC);LONG packed=(LONG)(scan<<16);
        if(key==VK_UP || key==VK_DOWN || key==VK_LEFT || key==VK_RIGHT || key==VK_INSERT || key==VK_DELETE || key==VK_HOME || key==VK_END || key==VK_PRIOR || key==VK_NEXT)packed|=1<<24;
        if(!GetKeyNameTextA(packed,name,sizeof(name)))snprintf(name,sizeof(name),"VK %d",key);}
    SetWindowTextA(button,name);
}
static HWND controls_child(HWND parent,const char *cls,const char *text,DWORD style,int x,int y,int width,int height,int id) {
    HWND child=CreateWindowExA(0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,parent,(HMENU)(INT_PTR)id,GetModuleHandleA(NULL),NULL);
    if(child)SendMessageA(child,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
    return child;
}
static LRESULT CALLBACK controls_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    ControlsEditor *e=(ControlsEditor *)GetWindowLongPtrA(window,GWLP_USERDATA);
    if(message==WM_NCCREATE) {e=((CREATESTRUCTA *)l)->lpCreateParams;SetWindowLongPtrA(window,GWLP_USERDATA,(LONG_PTR)e);}
    if(message==WM_CLOSE) {e->done=1;return 0;}
    if(message==WM_COMMAND && e) {
        unsigned id=LOWORD(w),i;
        if(id>=100 && id<109) {if(e->capture>=0)controls_label(e->keys[e->capture],e->value.keys[e->capture]);e->capture=(int)id-100;SetWindowTextA(e->keys[e->capture],"Press key (Esc cancels)");return 0;}
        if(id==IDOK) {
            BOOL valid;UINT zone=GetDlgItemInt(window,400,&valid,FALSE);int device=(int)SendMessageA(e->device,CB_GETCURSEL,0,0);
            if(!valid || zone>95 || device==CB_ERR)return 0;
            e->value.deadzone=(int)zone;e->value.device=device==0?-1:device-1;
            for(i=0;i<9;++i)e->value.buttons[i]=(int)SendMessageA(e->buttons[i],CB_GETCURSEL,0,0);
            e->saved=e->done=1;return 0;
        }
        if(id==IDCANCEL) {e->done=1;return 0;}
        if(id==500) {controls_defaults(&e->value);e->capture=-1;
            for(i=0;i<9;++i) {controls_label(e->keys[i],e->value.keys[i]);SendMessageA(e->buttons[i],CB_SETCURSEL,e->value.buttons[i],0);}
            SendMessageA(e->device,CB_SETCURSEL,0,0);SetDlgItemInt(window,400,(UINT)e->value.deadzone,FALSE);return 0;}
    }
    return DefWindowProcA(window,message,w,l);
}
static HWND controls_create(HWND parent,ControlsEditor *editor) {
    WNDCLASSA cls={0};ControlsEditor *e=editor;HWND window;unsigned i,j;
    cls.lpfnWndProc=controls_proc;cls.hInstance=GetModuleHandleA(NULL);cls.lpszClassName="BlazeControls";cls.hCursor=LoadCursor(NULL,IDC_ARROW);
    if(!RegisterClassA(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return NULL;
    e->capture=-1;
    window=CreateWindowExA(WS_EX_DLGMODALFRAME,cls.lpszClassName,"Blaze controls - F10",WS_CAPTION|WS_SYSMENU|WS_POPUP,CW_USEDEFAULT,CW_USEDEFAULT,490,405,parent,NULL,cls.hInstance,e);
    if(!window)return NULL;
    controls_child(window,"STATIC","Click a binding, then press a key. Esc cancels; Backspace unbinds.",0,12,10,455,20,0);
    controls_child(window,"STATIC","Controller",0,12,39,110,20,0);
    e->device=controls_child(window,"COMBOBOX","",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,130,36,210,220,300);
    SendMessageA(e->device,CB_ADDSTRING,0,(LPARAM)"Auto (first connected)");SendMessageA(e->device,CB_ADDSTRING,0,(LPARAM)"Off");
    for(i=1;i<=16;++i) {char label[24];snprintf(label,sizeof(label),"Joystick %u",i);SendMessageA(e->device,CB_ADDSTRING,0,(LPARAM)label);}
    SendMessageA(e->device,CB_SETCURSEL,e->value.device<0?0:e->value.device+1,0);
    controls_child(window,"STATIC","Deadzone %",0,345,39,90,20,0);
    e->deadzone=controls_child(window,"EDIT","",ES_NUMBER|WS_BORDER|WS_TABSTOP,430,36,38,22,400);SetDlgItemInt(window,400,(UINT)e->value.deadzone,FALSE);
    controls_child(window,"STATIC","Action",0,12,70,110,20,0);controls_child(window,"STATIC","Keyboard",0,130,70,190,20,0);controls_child(window,"STATIC","Pad button",0,345,70,120,20,0);
    for(i=0;i<9;++i) {
        int y=94+(int)i*25;
        controls_child(window,"STATIC",binding_names[i],0,12,y+3,115,22,0);
        e->keys[i]=controls_child(window,"BUTTON","",BS_PUSHBUTTON|WS_TABSTOP,130,y,205,23,100+(int)i);controls_label(e->keys[i],e->value.keys[i]);
        e->buttons[i]=controls_child(window,"COMBOBOX","",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,345,y,123,160,200+(int)i);
        SendMessageA(e->buttons[i],CB_ADDSTRING,0,(LPARAM)"None");
        for(j=1;j<=32;++j) {char label[20];snprintf(label,sizeof(label),"Button %u",j);SendMessageA(e->buttons[i],CB_ADDSTRING,0,(LPARAM)label);}
        SendMessageA(e->buttons[i],CB_SETCURSEL,e->value.buttons[i],0);
    }
    controls_child(window,"BUTTON","Defaults",BS_PUSHBUTTON|WS_TABSTOP,12,331,90,26,500);
    controls_child(window,"BUTTON","Save",BS_DEFPUSHBUTTON|WS_TABSTOP,260,331,96,26,IDOK);
    controls_child(window,"BUTTON","Cancel",BS_PUSHBUTTON|WS_TABSTOP,370,331,96,26,IDCANCEL);
    for(i=0;i<9;++i)if(!e->keys[i] || !e->buttons[i]) {DestroyWindow(window);return NULL;}
    if(!e->device || !e->deadzone || !GetDlgItem(window,IDOK) || !GetDlgItem(window,IDCANCEL)) {DestroyWindow(window);return NULL;}
    return window;
}
static void controls_capture(ControlsEditor *e,int key) {
    if(e->capture<0)return;
    if(key==VK_BACK)e->value.keys[e->capture]=0;
    else if(key!=VK_ESCAPE && key!=VK_F10 && key>0 && key<256)e->value.keys[e->capture]=key;
    controls_label(e->keys[e->capture],e->value.keys[e->capture]);e->capture=-1;
}
static int controls_edit(HWND parent,BlazeControls *value,BlazeWindowsAudio *audio) {
    ControlsEditor e={0};HWND window;MSG msg;
    e.value=*value;window=controls_create(parent,&e);if(!window)return -1;
    EnableWindow(parent,FALSE);ShowWindow(window,SW_SHOW);SetFocus(e.keys[0]);
    while(!e.done) {
        while(PeekMessageA(&msg,NULL,0,0,PM_REMOVE)) {
            if(msg.message==WM_QUIT) {e.done=1;e.saved=0;PostQuitMessage((int)msg.wParam);break;}
            if((msg.message==WM_KEYDOWN || msg.message==WM_SYSKEYDOWN) && e.capture>=0) {
                controls_capture(&e,(int)msg.wParam);continue;
            }
            if(!IsDialogMessageA(window,&msg)) {TranslateMessage(&msg);DispatchMessageA(&msg);}
            if(e.done)break;
        }
        /* Simulation is paused, but active SFX and MIDI keep their existing
           owners and continue pumping while the user edits bindings. */
        blaze_windows_audio_pump(audio);
        if(!e.done)MsgWaitForMultipleObjects(0,NULL,FALSE,20,QS_ALLINPUT);
    }
    DestroyWindow(window);if(IsWindow(parent)) {EnableWindow(parent,TRUE);SetActiveWindow(parent);}
    if(e.saved)*value=e.value;
    return e.saved?1:0;
}
#endif
