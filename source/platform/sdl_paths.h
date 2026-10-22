#ifndef BLAZE_SDL_PATHS_H
#define BLAZE_SDL_PATHS_H
#include "settings.h"
/* Assets remain immutable; a selected directory owns the complete original
   INIs as well as mutable options/scores/controls. NULL disables writes. */
int blaze_sdl_paths(BlazeSettings *,const char *assets,const char *requested,int hidden,int portable);
#endif
