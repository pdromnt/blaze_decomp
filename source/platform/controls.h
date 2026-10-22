#ifndef BLAZE_CONTROLS_H
#define BLAZE_CONTROLS_H
#include "blaze.h"
/* Persisted key IDs retain the existing Win32 VK numbering on every backend.
   SDL translates these IDs explicitly; they are never SDL scancodes. */
enum { BLAZE_BINDINGS=9, BLAZE_KEY_F10=0x79 };
extern const unsigned binding_actions[BLAZE_BINDINGS];
extern const char *const binding_names[BLAZE_BINDINGS];
extern const char *const binding_keys[BLAZE_BINDINGS];
extern const char *const binding_buttons[BLAZE_BINDINGS];
typedef struct {int keys[BLAZE_BINDINGS],buttons[BLAZE_BINDINGS],device,deadzone;} BlazeControls;
void controls_defaults(BlazeControls *controls);
void controls_key(BlazeControls *controls,const char *key,int value);
unsigned controls_keyboard(const BlazeControls *controls,const unsigned char down[256]);
#endif
