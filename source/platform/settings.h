#ifndef BLAZE_SETTINGS_H
#define BLAZE_SETTINGS_H
#include "controls.h"
typedef struct {
    BlazeControls controls;
    char path[1200],score_path[1200],controls_path[1200];
} BlazeSettings;
/* Directory must already exist, or be NULL to disable writes. No global owner. */
int blaze_settings_paths(BlazeSettings *settings,const char *directory);
int blaze_settings_load(BlazeSettings *settings,BlazeApp *app);
int blaze_settings_options(BlazeSettings *settings,int level,int display,int music);
int blaze_settings_progress(BlazeSettings *settings,const int scores[5],const int modes[5],unsigned weapons);
int blaze_settings_controls(BlazeSettings *settings);
#endif
