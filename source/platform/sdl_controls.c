#include "sdl_controls.h"
#include "sdl_keys.h"
#include <stdio.h>
#include <string.h>

static void editor_pad(BlazeSdlControls *e) {
    SDL_JoystickID *ids;int count,index;
    if(e->pad && !SDL_JoystickConnected(e->pad)) {SDL_CloseJoystick(e->pad);e->pad=NULL;e->capture=-1;}
    if(e->pad || !e->value.device)return;
    ids=SDL_GetJoysticks(&count);index=e->value.device<0?0:e->value.device-1;
    if(ids && index<count)e->pad=SDL_OpenJoystick(ids[index]);
    SDL_free(ids);
}
void blaze_sdl_controls_open(BlazeSdlControls *e,const BlazeControls *value) {
    memset(e,0,sizeof(*e));e->value=*value;e->active=1;e->capture=-1;
}
void blaze_sdl_controls_close(BlazeSdlControls *e) {
    if(e->pad)SDL_CloseJoystick(e->pad);memset(e,0,sizeof(*e));e->capture=-1;
}
void blaze_sdl_controls_focus(BlazeSdlControls *e) {e->capture=-1;e->wait_release=1;}
static int save(BlazeSdlControls *e,BlazeSettings *applied) {
    BlazeSettings candidate=*applied;
    candidate.controls=e->value;
    if(!blaze_settings_controls(&candidate)) {e->failed=1;return 0;}
    applied->controls=e->value;blaze_sdl_controls_close(e);return 1;
}
static int activate(BlazeSdlControls *e,BlazeSettings *applied,int wait_release) {
    if(e->row>=2 && e->row<=10) {e->capture=e->column;e->wait_release=wait_release;editor_pad(e);}
    else if(e->row==11) {controls_defaults(&e->value);if(e->pad)SDL_CloseJoystick(e->pad);e->pad=NULL;e->capture=-1;e->wait_release=0;}
    else if(e->row==12)return save(e,applied);
    else if(e->row==13) {blaze_sdl_controls_close(e);return -1;}
    return 0;
}
static void adjust(BlazeSdlControls *e,int delta) {
    if(e->row==0) {
        e->value.device+=delta;if(e->value.device<-1)e->value.device=16;if(e->value.device>16)e->value.device=-1;
        if(e->pad)SDL_CloseJoystick(e->pad);e->pad=NULL;editor_pad(e);
    } else if(e->row==1)e->value.deadzone=SDL_clamp(e->value.deadzone+delta*5,0,95);
    else if(e->row<=10)e->column=delta>0?1:0;
}
int blaze_sdl_controls_click(BlazeSdlControls *e,BlazeSettings *applied,SDL_Renderer *r,float x,float y) {
    int width,height;float scale;
    if(!e->active || !SDL_GetRenderOutputSize(r,&width,&height) || !SDL_RenderCoordinatesFromWindow(r,x,y,&x,&y))return 0;
    scale=SDL_min(width/320.0f,height/240.0f);if(scale<=0)return 0;
    x=x/scale-(width/scale-320)/2;y=y/scale-(height/scale-240)/2;
    if(x<8 || x>=312)return 0;
    if(y>=184 && y<196) {
        if(x<100)e->row=11;else if(x>=128 && x<200)e->row=12;else if(x>=232)e->row=13;else return 0;
        return activate(e,applied,0);
    }
    if(e->capture>=0)return 0;
    e->failed=0;
    if(y>=28 && y<48) {e->row=y<40?0:1;adjust(e,x<160?-1:1);}
    else if(y>=68 && y<176) {e->row=2+(int)(y-68)/12;e->column=x>=256;return activate(e,applied,0);}
    return 0;
}
int blaze_sdl_controls_event(BlazeSdlControls *e,BlazeSettings *applied,const SDL_Event *event) {
    SDL_Scancode scan;int delta=0;
    if(!e->active)return 0;
    editor_pad(e);
    if(event->type==SDL_EVENT_JOYSTICK_REMOVED) {
        if(e->pad && event->jdevice.which==SDL_GetJoystickID(e->pad)) {
            SDL_CloseJoystick(e->pad);e->pad=NULL;e->capture=-1;
        }return 0;
    }
    if(event->type==SDL_EVENT_KEY_UP || event->type==SDL_EVENT_JOYSTICK_BUTTON_UP) {e->wait_release=0;return 0;}
    if(e->capture==1 && event->type==SDL_EVENT_JOYSTICK_BUTTON_DOWN && e->pad &&
       event->jbutton.which==SDL_GetJoystickID(e->pad) && event->jbutton.button<32 && !e->wait_release) {
        e->value.buttons[e->row-2]=event->jbutton.button+1;e->capture=-1;return 0;
    }
    if(event->type!=SDL_EVENT_KEY_DOWN || event->key.repeat || e->wait_release)return 0;
    scan=event->key.scancode;
    if(e->capture>=0) {
        int key=key_id(scan);
        if(scan==SDL_SCANCODE_ESCAPE || scan==SDL_SCANCODE_F10)e->capture=-1;
        else if(scan==SDL_SCANCODE_BACKSPACE) {
            if(e->capture)e->value.buttons[e->row-2]=0;else e->value.keys[e->row-2]=0;
            e->capture=-1;
        } else if(e->capture==0 && key) {e->value.keys[e->row-2]=key;e->capture=-1;}
        return 0;
    }
    e->failed=0;
    if(scan==SDL_SCANCODE_ESCAPE || scan==SDL_SCANCODE_F10) {blaze_sdl_controls_close(e);return -1;}
    if(scan==SDL_SCANCODE_UP)e->row=(e->row+13)%14;
    if(scan==SDL_SCANCODE_DOWN)e->row=(e->row+1)%14;
    if(scan==SDL_SCANCODE_TAB) {
        if(e->row>=2 && e->row<=10 && !e->column)e->column=1;
        else {e->column=0;e->row=(e->row+1)%14;}
    }
    if(scan==SDL_SCANCODE_LEFT)delta=-1;
    if(scan==SDL_SCANCODE_RIGHT)delta=1;
    if(delta)adjust(e,delta);
    if(scan==SDL_SCANCODE_RETURN || scan==SDL_SCANCODE_KP_ENTER) {
        return activate(e,applied,1);
    }
    /* Pad bindings can also be selected without a connected controller. */
    if(e->row>=2 && e->row<=10 && e->column && (scan==SDL_SCANCODE_PAGEUP || scan==SDL_SCANCODE_PAGEDOWN)) {
        int *button=&e->value.buttons[e->row-2];*button=SDL_clamp(*button+(scan==SDL_SCANCODE_PAGEUP?1:-1),0,32);
    }
    return 0;
}
static int label(SDL_Renderer *r,float x,float y,const char *text,int selected) {
    return SDL_SetRenderDrawColor(r,selected?255:180,selected?230:200,selected?80:220,255) && SDL_RenderDebugText(r,x,y,text);
}
int blaze_sdl_controls_draw(BlazeSdlControls *e,SDL_Renderer *r) {
    static const char *const names[]={"UP","DOWN","LEFT","RIGHT","FIRE / CONFIRM","ABILITY / BACK","PAUSE","MAIN MENU","QUIT"};
    int width,height,result=1;float old_x,old_y,scale,x,y;unsigned i;char text[48];
    SDL_BlendMode old_blend;SDL_FRect panel;
    if(!e->active)return 1;editor_pad(e);
    if(!SDL_GetRenderOutputSize(r,&width,&height) || !SDL_GetRenderScale(r,&old_x,&old_y) || !SDL_GetRenderDrawBlendMode(r,&old_blend))return 0;
    scale=SDL_min(width/320.0f,height/240.0f);if(scale<=0)return 1;
    x=(width/scale-320)/2;y=(height/scale-240)/2;panel=(SDL_FRect){x,y,320,240};
    if(!SDL_SetRenderScale(r,scale,scale) || !SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE) ||
       !SDL_SetRenderDrawColor(r,8,12,20,255) || !SDL_RenderFillRect(r,&panel))result=0;
    result&=label(r,x+56,y+8,"BLAZE CONTROLS - F10",1);
    if(e->value.device<0)snprintf(text,sizeof(text),"CONTROLLER: AUTO");
    else if(!e->value.device)snprintf(text,sizeof(text),"CONTROLLER: OFF");
    else snprintf(text,sizeof(text),"CONTROLLER: JOYSTICK %d",e->value.device);
    result&=label(r,x+8,y+28,text,e->row==0);
    snprintf(text,sizeof(text),"DEADZONE: %d%%",e->value.deadzone);result&=label(r,x+8,y+40,text,e->row==1);
    result&=label(r,x+8,y+56,"ACTION          KEY             PAD",0);
    for(i=0;i<BLAZE_BINDINGS;++i) {
        const char *name=e->value.keys[i]?SDL_GetScancodeName(key_scan(e->value.keys[i])):"UNBOUND";unsigned j;
        snprintf(text,sizeof(text),"%.14s",name);for(j=0;text[j];++j)if(text[j]>='a' && text[j]<='z')text[j]-='a'-'A';
        result&=label(r,x+8,y+68+i*12,names[i],0);
        result&=label(r,x+136,y+68+i*12,text,e->row==(int)i+2 && !e->column);
        snprintf(text,sizeof(text),"%d",e->value.buttons[i]);result&=label(r,x+272,y+68+i*12,text,e->row==(int)i+2 && e->column);
    }
    result&=label(r,x+8,y+184,"DEFAULTS",e->row==11);
    result&=label(r,x+136,y+184,"SAVE",e->row==12);
    result&=label(r,x+240,y+184,"CANCEL",e->row==13);
    if(e->failed)result&=label(r,x+8,y+200,"SAVE FAILED - OLD BINDINGS RETAINED",1);
    else if(e->capture>=0)result&=label(r,x+8,y+200,e->capture?"PRESS PAD BUTTON; BACKSPACE CLEARS":"PRESS KEY; BACKSPACE CLEARS",1);
    else result&=label(r,x+8,y+200,"ARROWS/TAB MOVE; ENTER EDITS",0);
    result&=label(r,x+8,y+212,e->capture>=0?"ESC CANCELS CAPTURE":"PAD: PGUP/PGDN SET BUTTON (0=NONE)",0);
    result&=label(r,x+8,y+224,"ESC/F10 CANCEL; SAVE APPLIES",0);
    if(!SDL_SetRenderScale(r,old_x,old_y) || !SDL_SetRenderDrawBlendMode(r,old_blend))result=0;
    return result;
}
