/* Exercise the actual Win32 backend with a hidden owned window and memory DC.
   No game process, keyboard/mouse input, desktop capture or focus changes. */
#define wmain blaze_windows_entry
#include "../../platform/windows.c"
#undef wmain

#define CHECK(condition) do {if(!(condition)) {fprintf(stderr,"line %d: %s\n",__LINE__,#condition);goto done;}} while(0)
int main(int argc,char **argv) {
    BlazeFrame *frame=calloc(1,sizeof(*frame));WNDCLASSA cls={0};HWND window=NULL;
    HDC dc=NULL;HBITMAP bitmap=NULL;HGDIOBJ previous=NULL;void *pixels=NULL;
    BITMAPINFO info={0};HWND foreground=GetForegroundWindow();unsigned y;int result=1;
    WindowSettings settings={0};BlazeApp *options=calloc(1,sizeof(*options)),*reopened=NULL;
    char temporary[MAX_PATH]="",folder[MAX_PATH],testdir[MAX_PATH]="";RECT bounds;DEVMODEA before={0},after={0};
    CHECK(argc==3 && frame && options && blaze_app_open(options,argv[1],argv[2],NULL));
    {FrameClock clock={1000,40};
        CHECK(frame_wait(&clock,999)==1 && !frame_wait(&clock,1000));
        frame_advance(&clock,1007);CHECK(clock.next==1040 && frame_wait(&clock,1007)==33);
        frame_advance(&clock,1200);CHECK(clock.next==1200 && !frame_wait(&clock,1200));
        frame_advance(&clock,1203);CHECK(clock.next==1240); /* No catch-up burst. */
    }
    frame->palette[4]=0x33;frame->palette[5]=0x66;frame->palette[6]=0xcc;
    frame->palette[8]=0x44;frame->palette[9]=0x22;frame->palette[10]=0x11;
    memset(frame->pixels,1,sizeof(frame->pixels));
    for(y=0;y<BLAZE_HEIGHT;++y)memset(frame->pixels+y*BLAZE_WIDTH+BLAZE_WIDTH/2,2,BLAZE_WIDTH/2);
    cls.lpfnWndProc=window_proc;cls.hInstance=GetModuleHandleA(NULL);cls.lpszClassName="BlazeNative";
    cls.hIcon=LoadIconA(cls.hInstance,MAKEINTRESOURCEA(BLAZE_ICON));CHECK(cls.hIcon);
    CHECK(RegisterClassA(&cls));window=create_game_window(cls.hInstance,frame);CHECK(window);
    CHECK(!(GetWindowLongPtrA(window,GWL_EXSTYLE)&WS_EX_NOACTIVATE));
    CHECK((BlazeFrame *)GetWindowLongPtrA(window,GWLP_USERDATA)==frame);
    CHECK(GetForegroundWindow()==foreground);
    {char title[32];CHECK(GetWindowTextA(window,title,sizeof(title)) && !strcmp(title,"BLAZE"));}
    CHECK((HICON)GetClassLongPtrA(window,GCLP_HICON)==cls.hIcon);
    CHECK((HICON)SendMessageA(window,WM_GETICON,ICON_BIG,0)==cls.hIcon);
    CHECK(SendMessageA(window,WM_GETICON,ICON_SMALL,0)!=0);
    settings.window=window;CHECK(SetPropA(window,"BlazeSettings",&settings));
    before.dmSize=sizeof(before);CHECK(EnumDisplaySettingsA(NULL,ENUM_CURRENT_SETTINGS,&before));
    for(y=0;y<10;++y) {
        int mode=(int)(y%5);CHECK(apply_display(&settings,mode));
        CHECK(GetClientRect(window,&bounds));
        if(mode<2)CHECK(bounds.right==320*(mode+1) && bounds.bottom==240*(mode+1));
        else CHECK((GetWindowLongPtrA(window,GWL_STYLE)&WS_POPUP)!=0);
        CHECK(!IsWindowVisible(window) && GetForegroundWindow()==foreground);
    }
    {
        BlazeControls c;BlazePad pad={-1,0};unsigned char down[256]={0};
        JOYCAPSA caps={0};JOYINFOEX sample={0};unsigned i;
        controls_defaults(&c);
        for(i=0;i<BLAZE_BINDINGS;++i) {
            memset(down,0,sizeof(down));down[c.keys[i]]=1;CHECK(controls_keyboard(&c,down)==binding_actions[i]);
        }
        controls_key(&c,"KeyFire",'A');memset(down,0,sizeof(down));down['Z']=1;CHECK(!controls_keyboard(&c,down));
        down['A']=1;CHECK(controls_keyboard(&c,down)==BLAZE_CONFIRM);
        controls_key(&c,"KeyFire",256);controls_key(&c,"KeyFire",VK_F10);CHECK(c.keys[4]=='A');
        controls_key(&c,"ButtonFire",32);controls_key(&c,"ButtonFire",33);CHECK(c.buttons[4]==32);
        caps.wXmin=caps.wYmin=100;caps.wXmax=caps.wYmax=2100;caps.wCaps=JOYCAPS_HASPOV;
        sample.dwXpos=sample.dwYpos=1100;sample.dwPOV=JOY_POVCENTERED;
        CHECK(!controls_pad(&c,&caps,&sample));
        sample.dwXpos=850;CHECK(!controls_pad(&c,&caps,&sample));sample.dwXpos=849;CHECK(controls_pad(&c,&caps,&sample)==BLAZE_LEFT);
        sample.dwXpos=1351;CHECK(controls_pad(&c,&caps,&sample)==BLAZE_RIGHT);sample.dwXpos=1100;
        sample.dwYpos=849;CHECK(controls_pad(&c,&caps,&sample)==BLAZE_UP);sample.dwYpos=1351;CHECK(controls_pad(&c,&caps,&sample)==BLAZE_DOWN);sample.dwYpos=1100;
        {static const unsigned hats[]={BLAZE_UP,BLAZE_UP|BLAZE_RIGHT,BLAZE_RIGHT,BLAZE_DOWN|BLAZE_RIGHT,BLAZE_DOWN,BLAZE_DOWN|BLAZE_LEFT,BLAZE_LEFT,BLAZE_UP|BLAZE_LEFT};
            for(i=0;i<8;++i) {sample.dwPOV=i*4500;CHECK(controls_pad(&c,&caps,&sample)==hats[i]);}}
        sample.dwPOV=JOY_POVCENTERED;sample.dwButtons=0x80000000u;CHECK(controls_pad(&c,&caps,&sample)==BLAZE_CONFIRM);
        CHECK(!controls_poll(window,&c,&pad)); /* Hidden window never polls real input. */
        {
            ControlsEditor e={0};HWND editor;e.value=c;editor=controls_create(window,&e);CHECK(editor);
            CHECK(!IsWindowVisible(editor) && GetForegroundWindow()==foreground);
            SendMessageA(editor,WM_COMMAND,104,0);controls_capture(&e,VK_BACK);CHECK(!e.value.keys[4]);
            SendMessageA(editor,WM_COMMAND,104,0);CHECK(e.capture==4);controls_capture(&e,'S');CHECK(e.value.keys[4]=='S');
            SendMessageA(editor,WM_COMMAND,104,0);controls_capture(&e,VK_ESCAPE);CHECK(e.value.keys[4]=='S');
            SetDlgItemInt(editor,400,120,FALSE);SendMessageA(editor,WM_COMMAND,IDOK,0);CHECK(!e.done);
            SetDlgItemInt(editor,400,30,FALSE);SendMessageA(e.device,CB_SETCURSEL,3,0); /* Joystick 2 */
            SendMessageA(e.buttons[4],CB_SETCURSEL,3,0);SendMessageA(editor,WM_COMMAND,IDOK,0);
            CHECK(e.saved && e.value.device==2 && e.value.deadzone==30 && e.value.buttons[4]==3);DestroyWindow(editor);
            memset(&e,0,sizeof(e));e.value=c;editor=controls_create(window,&e);CHECK(editor);
            SendMessageA(editor,WM_COMMAND,500,0);CHECK(!e.done && e.value.keys[4]=='Z');
            SendMessageA(editor,WM_COMMAND,IDCANCEL,0);CHECK(e.done && !e.saved);DestroyWindow(editor);
        }
    }
    CHECK(apply_display(&settings,1));
    after.dmSize=sizeof(after);CHECK(EnumDisplaySettingsA(NULL,ENUM_CURRENT_SETTINGS,&after));
    CHECK(before.dmPelsWidth==after.dmPelsWidth && before.dmPelsHeight==after.dmPelsHeight &&
          before.dmDisplayFrequency==after.dmDisplayFrequency && before.dmBitsPerPel==after.dmBitsPerPel);
    CHECK(GetTempPathA(sizeof(folder),folder) && GetTempFileNameA(folder,"blz",0,testdir));
    CHECK(DeleteFileA(testdir) && CreateDirectoryA(testdir,NULL));
    CHECK(settings_location(&settings,folder,testdir,0));strcpy(temporary,settings.storage.path);
    {FILE *old=fopen(temporary,"wb");CHECK(old);
        CHECK(fputs("[Enh]\nLevel=2\nScreen=0\nMidPlayer=1\nTime=40\nWeaponName4=LIGHTNING\n[Foreign]\nKeep=untouched\n",old)>=0);CHECK(!fclose(old));}
    {FILE *old=fopen(settings.storage.score_path,"wb");CHECK(old);
        CHECK(fputs("[Score]\nClearMode1=1\nClearMode2=2\nClearMode3=3\nClearMode4=0\nClearMode5=4\nID=unchanged\n",old)>=0);CHECK(!fclose(old));}
    CHECK(load_options(&settings,options));
    CHECK(options->level==2 && options->ranking_scores[0]==6000 && options->loadout.weapons[1].enabled);
    CHECK(options->ranking_modes[0]==1 && options->ranking_modes[4]==4);
    controls_key(&settings.storage.controls,"KeyFire",'A');controls_key(&settings.storage.controls,"ButtonFire",32);
    CHECK(save_controls(&settings) && save_options(&settings,2,0,1) && load_options(&settings,options));
    CHECK(settings.storage.controls.keys[4]=='A' && settings.storage.controls.buttons[4]==32);
    CHECK(options->level==2 && options->display==0 && options->music==1);
    CHECK(save_options(&settings,0,1,2) && load_options(&settings,options));
    CHECK(options->level==0 && options->display==1 && options->music==2);
    {
        WindowSettings location={0};char expected[MAX_PATH],absolute[MAX_PATH],data[MAX_PATH];
        CHECK(blaze_path(data,sizeof(data),folder,"dat") && GetFullPathNameA(data,sizeof(absolute),absolute,NULL));
        CHECK(blaze_path(expected,sizeof(expected),absolute,"Blz.ini"));
        CHECK(settings_location(&location,folder,NULL,0) && !strcmp(location.storage.path,expected));
        CHECK(settings_location(&location,folder,NULL,1) && !location.storage.path[0]);
        CHECK(settings_location(&location,folder,"none",0) && !location.storage.path[0]);
        CHECK(settings_location(&location,folder,testdir,1) && !strcmp(location.storage.path,temporary));
        CHECK(!strcmp(location.storage.score_path,settings.storage.score_path) && !strcmp(location.storage.controls_path,settings.storage.controls_path));
    }
    {
        WindowSettings next={0};HANDLE lock;
        reopened=calloc(1,sizeof(*reopened));
        CHECK(reopened && settings_location(&next,folder,testdir,1));
        options->settings_context=&settings;options->save_progress=save_progress;
        options->state=BLAZE_STAGE_CLEAR;options->tick=128;options->level_data.stage=0;
        options->world.enemies_created=1; /* Report-entry prerequisite. */
        options->loadout.weapons[3].enabled=0;options->world.player.words[0x94/4]=0;
        CHECK(blaze_app_tick(options,0) && options->state==BLAZE_LOADING);
        CHECK(blaze_app_open(reopened,argv[1],argv[2],NULL) && load_options(&next,reopened));
        CHECK(reopened->loadout.weapons[3].enabled && reopened->level==0 && reopened->music==2);
        CHECK(next.storage.controls.keys[4]=='A' && next.storage.controls.buttons[4]==32);
        blaze_app_close(reopened);
        /* Ending score insertion persists immediately, before unlock/exit. */
        options->state=BLAZE_ENDING;options->tick=18;options->loadout.mode=4;
        options->world.player.words[0x58/4]=7000;
        CHECK(blaze_app_tick(options,0) && options->ranking_scores[0]==7000);
        CHECK(blaze_app_open(reopened,argv[1],argv[2],NULL) && load_options(&next,reopened));
        CHECK(reopened->high_score==7000 && reopened->ranking_scores[1]==6000);
        {static const int expected[]={4,1,2,3,0};CHECK(!memcmp(reopened->ranking_modes,expected,sizeof(expected)));}
        blaze_app_close(reopened);
        options->tick=260;options->previous_input=0;options->loadout.weapons[8].enabled=0;
        options->loadout.weapons[7].enabled=1;
        CHECK(blaze_app_tick(options,BLAZE_CONFIRM) && options->state==BLAZE_RANKING);
        /* Option changes preserve all progress; progress preserves the saved
           full-screen choice even though startup's real window is windowed. */
        CHECK(save_options(&settings,1,4,2));options->display=1;
        CHECK(blaze_progress_save(options));
        CHECK(blaze_app_open(reopened,argv[1],argv[2],NULL) && load_options(&next,reopened));
        CHECK(reopened->ranking_scores[0]==7000 && reopened->loadout.weapons[8].enabled);
        CHECK(reopened->ranking_modes[0]==4 && reopened->ranking_modes[1]==1 && reopened->ranking_modes[4]==0);
        CHECK(reopened->level==1 && reopened->display==4 && reopened->music==2);
        blaze_app_close(reopened);
        lock=CreateFileA(settings.storage.score_path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        CHECK(lock!=INVALID_HANDLE_VALUE);
        options->loadout.weapons[9].enabled=1;
        options->ranking_scores[4]=3500;options->ranking_modes[4]=2;
        {int saved=blaze_progress_save(options);CloseHandle(lock);CHECK(!saved && strstr(options->error,"save user progress"));}
        CHECK(blaze_app_open(reopened,argv[1],argv[2],NULL) && load_options(&next,reopened));
        CHECK(!reopened->loadout.weapons[9].enabled && reopened->ranking_scores[0]==7000);
        CHECK(reopened->ranking_scores[4]==3000 && reopened->ranking_modes[4]==0);
        blaze_app_close(reopened);
        CHECK(blaze_progress_save(options));
        CHECK(blaze_app_open(reopened,argv[1],argv[2],NULL) && load_options(&next,reopened));
        CHECK(reopened->loadout.weapons[9].enabled);
        CHECK(reopened->ranking_scores[4]==3500 && reopened->ranking_modes[4]==2);
        blaze_app_close(reopened);
        {FILE *bad=fopen(settings.storage.score_path,"ab");CHECK(bad);
            CHECK(fputs("[Other]\nHiScore1=1\nClearMode1=0\n[Score]\nHiScore2=999999999999999999999\nWeaponFlag9=2\nHiScore1=1oops\nClearMode1=1oops\nClearMode2=999999999999999999999\n",bad)>=0);CHECK(!fclose(bad));}
        CHECK(blaze_app_open(reopened,argv[1],argv[2],NULL) && load_options(&next,reopened));
        CHECK(reopened->ranking_scores[0]==7000 && reopened->ranking_scores[1]==6000 && reopened->loadout.weapons[9].enabled);
        CHECK(reopened->ranking_modes[0]==4 && reopened->ranking_modes[1]==1);
        blaze_app_close(reopened);free(reopened);reopened=NULL;
        {
            char value[80];
            CHECK(GetPrivateProfileIntA("Enh","Time",0,temporary)==40);
            CHECK(GetPrivateProfileStringA("Enh","WeaponName4","",value,sizeof(value),temporary) && !strcmp(value,"LIGHTNING"));
            CHECK(GetPrivateProfileStringA("Foreign","Keep","",value,sizeof(value),temporary) && !strcmp(value,"untouched"));
            CHECK(GetPrivateProfileIntA("Score","ClearMode1",0,settings.storage.score_path)==4);
            CHECK(GetPrivateProfileStringA("Score","ID","",value,sizeof(value),settings.storage.score_path) && !strcmp(value,"unchanged"));
            CHECK(GetPrivateProfileStringA("Input","KeyFire","absent",value,sizeof(value),temporary) && !strcmp(value,"absent"));
            CHECK(GetPrivateProfileStringA("Score","HiScore1","absent",value,sizeof(value),temporary) && !strcmp(value,"absent"));
            CHECK(GetPrivateProfileStringA("Enh","Level","absent",value,sizeof(value),settings.storage.controls_path) && !strcmp(value,"absent"));
        }
        /* Controls are independently atomic; a locked controls file must not
           affect options/progress or replace the previous bindings. */
        lock=CreateFileA(settings.storage.controls_path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);CHECK(lock!=INVALID_HANDLE_VALUE);
        controls_key(&settings.storage.controls,"KeyFire",'B');
        {int saved=save_controls(&settings);CloseHandle(lock);CHECK(!saved);}
        CHECK(load_options(&settings,options) && settings.storage.controls.keys[4]=='A');
        CHECK(settings_location(&settings,folder,"none",0) && blaze_progress_save(options));
        CHECK(settings_location(&settings,folder,testdir,0));
    }
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=640;
    info.bmiHeader.biHeight=-480;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    info.bmiHeader.biCompression=BI_RGB;
    dc=CreateCompatibleDC(NULL);CHECK(dc);
    bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,NULL,0);CHECK(bitmap && pixels);
    previous=SelectObject(dc,bitmap);CHECK(previous && previous!=HGDI_ERROR);
    memset(pixels,255,640*480*4);
    SendMessageA(window,WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);
    CHECK(GetPixel(dc,10,10)==RGB(0xcc,0x66,0x33));
    CHECK(GetPixel(dc,630,470)==RGB(0x11,0x22,0x44));
    /* A paused/unfocused window repaints changed retained pixels without a tick. */
    memset(frame->pixels,2,sizeof(frame->pixels));
    SendMessageA(window,WM_PRINTCLIENT,(WPARAM)dc,PRF_CLIENT);
    CHECK(GetPixel(dc,10,10)==RGB(0x11,0x22,0x44));
    CHECK(GetForegroundWindow()==foreground);
    puts("Native display/repaint, separate legacy INIs/controls, unknown-key preservation, reload and failed-write recovery passed");result=0;
done:
    if(previous && previous!=HGDI_ERROR)SelectObject(dc,previous);
    if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);
    if(window) {RemovePropA(window,"BlazeSettings");DestroyWindow(window);}UnregisterClassA("BlazeNative",GetModuleHandleA(NULL));
    if(testdir[0]) {DeleteFileA(temporary);DeleteFileA(settings.storage.score_path);DeleteFileA(settings.storage.controls_path);RemoveDirectoryA(testdir);}
    if(reopened)blaze_app_close(reopened);free(reopened);
    if(options)blaze_app_close(options);free(options);free(frame);return result;
}
