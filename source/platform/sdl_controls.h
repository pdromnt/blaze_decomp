#ifndef BLAZE_SDL_CONTROLS_H
#define BLAZE_SDL_CONTROLS_H
#include "settings.h"
#include <SDL3/SDL.h>
/* This temporary value belongs only to the editor. Save writes it before
   replacing applied controls; no per-frame synchronization or game mirror. */
typedef struct {
    BlazeControls value;
    SDL_Joystick *pad;
    int active,row,column,capture,failed,wait_release;
} BlazeSdlControls;
void blaze_sdl_controls_open(BlazeSdlControls *,const BlazeControls *);
void blaze_sdl_controls_close(BlazeSdlControls *);
/* Returns 1 only after successful Save, -1 on Cancel, otherwise 0. */
int blaze_sdl_controls_event(BlazeSdlControls *,BlazeSettings *,const SDL_Event *);
int blaze_sdl_controls_click(BlazeSdlControls *,BlazeSettings *,SDL_Renderer *,float x,float y);
void blaze_sdl_controls_focus(BlazeSdlControls *);
int blaze_sdl_controls_draw(BlazeSdlControls *,SDL_Renderer *);
#endif
