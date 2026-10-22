#include "controls.h"
#include <string.h>
const unsigned binding_actions[BLAZE_BINDINGS]={BLAZE_UP,BLAZE_DOWN,BLAZE_LEFT,BLAZE_RIGHT,BLAZE_CONFIRM,BLAZE_CANCEL,BLAZE_PAUSE,BLAZE_MAIN,BLAZE_QUIT};
const char *const binding_names[BLAZE_BINDINGS]={"Up","Down","Left","Right","Fire / Confirm","Ability / Cancel","Pause","Main menu","Quit"};
const char *const binding_keys[BLAZE_BINDINGS]={"KeyUp","KeyDown","KeyLeft","KeyRight","KeyFire","KeyAbility","KeyPause","KeyMain","KeyQuit"};
const char *const binding_buttons[BLAZE_BINDINGS]={"ButtonUp","ButtonDown","ButtonLeft","ButtonRight","ButtonFire","ButtonAbility","ButtonPause","ButtonMain","ButtonQuit"};
void controls_defaults(BlazeControls *c) {
    static const int keys[]={0x26,0x28,0x25,0x27,'Z','X',0x1b,0x71,0x78};
    memset(c,0,sizeof(*c));memcpy(c->keys,keys,sizeof(keys));c->device=-1;c->deadzone=25;
    c->buttons[4]=1;c->buttons[5]=2;c->buttons[6]=8;
}
void controls_key(BlazeControls *c,const char *key,int value) {
    unsigned i;
    if(!strcmp(key,"PadDevice") && value>=-1 && value<=16)c->device=value;
    if(!strcmp(key,"Deadzone") && value>=0 && value<=95)c->deadzone=value;
    for(i=0;i<BLAZE_BINDINGS;++i) {
        if(!strcmp(key,binding_keys[i]) && value>=0 && value<=255 && value!=BLAZE_KEY_F10)c->keys[i]=value;
        if(!strcmp(key,binding_buttons[i]) && value>=0 && value<=32)c->buttons[i]=value;
    }
}
unsigned controls_keyboard(const BlazeControls *c,const unsigned char down[256]) {
    unsigned i,mask=0;
    for(i=0;i<BLAZE_BINDINGS;++i)if(c->keys[i] && down[c->keys[i]])mask|=binding_actions[i];
    return mask;
}
