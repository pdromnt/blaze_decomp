#ifndef BLAZE_MUSIC_STATUS_H
#define BLAZE_MUSIC_STATUS_H
#include <stdint.h>
typedef struct {
    uint64_t submitted,completed,starts,loops;
    int playing,finished,active,valid[2],failed;
} BlazeMusicStatus;
#endif
