#ifndef BLAZE_SDL_KEYS_H
#define BLAZE_SDL_KEYS_H
#include <SDL3/SDL.h>
/* Fixed storage format, independent of SDL physical scancode numbering. */
static inline SDL_Scancode key_scan(int key) {
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
static inline int key_down(const bool *keys,int key) {
    SDL_Scancode scan=key_scan(key);
    if(key==0x10)return keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    if(key==0x11)return keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    if(key==0x12)return keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT];
    if(key==13)return keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_KP_ENTER];
    return scan!=SDL_SCANCODE_UNKNOWN && keys[scan];
}

/* Reverse only the supported legacy IDs; reserve F10 for the editor. */
static inline int key_id(SDL_Scancode scan) {
    int key;
    if(scan==SDL_SCANCODE_UNKNOWN)return 0;
    if(scan==SDL_SCANCODE_RSHIFT)return 0x10;
    if(scan==SDL_SCANCODE_RCTRL)return 0x11;
    if(scan==SDL_SCANCODE_RALT)return 0x12;
    if(scan==SDL_SCANCODE_KP_ENTER)return 13;
    for(key=1;key<256;++key)if(key_scan(key)==scan)return key;
    return 0;
}
#endif
