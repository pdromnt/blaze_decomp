#include "blaze.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void blaze_level_close(BlazeLevel *level) {
    size_t i;
    blaze_archive_free(&level->objects); blaze_archive_free(&level->tiles);
    for (i=0;i<3;++i) free(level->map[i]);
    memset(level,0,sizeof(*level));
}
/* File-backed part of 0x407220. Decode __getw's four-byte little-endian
   words explicitly. Retain unknown spawn values as bit patterns; coordinate
   shifts wrap as on x86. Actor registration is a separate, unrecovered gate. */
int blaze_level_open(BlazeLevel *out, const char *root, unsigned stage,
                     unsigned difficulty, char *error, size_t capacity) {
    BlazeLevel next={0}; char path[1200],relative[80];
    uint8_t *bytes=NULL; size_t size,offset=0,layer,i,expected=0;
    const char *reason="Invalid level selection";
    if (stage>=5 || difficulty>=3) goto fail;
    next.stage=stage; next.difficulty=difficulty;
    if (!blaze_path(path,sizeof(path),root,"dar/BLAZE.DAR") ||
        !blaze_archive_read(&next.objects,path,error,capacity)) goto resource_fail;
    snprintf(relative,sizeof(relative),"dar/BLZ_MAP%u.DAR",stage+1);
    if (!blaze_path(path,sizeof(path),root,relative) ||
        !blaze_archive_read(&next.tiles,path,error,capacity)) goto resource_fail;
    for (layer=0;layer<3;++layer) {
        next.width[layer]=2560/(layer+2); next.height[layer]=70/(layer+5);
        expected+=next.width[layer]*next.height[layer]*4;
    }
    snprintf(relative,sizeof(relative),"dat/blz_m%u.dat",stage+1);
    reason="Missing or truncated tile map";
    if (!blaze_path(path,sizeof(path),root,relative) || !blaze_file_read(path,&bytes,&size) ||
        size!=expected) goto fail;
    for (layer=0;layer<3;++layer) {
        size_t count=next.width[layer]*next.height[layer];
        next.map[layer]=malloc(count*sizeof(*next.map[layer]));
        if (!next.map[layer]) { reason="Level allocation failed"; goto fail; }
        for (i=0;i<count;++i,offset+=4) next.map[layer][i]=blaze_le32(bytes+offset);
    }
    free(bytes); bytes=NULL;
    snprintf(relative,sizeof(relative),"dat/blz_e%u%u.dat",stage+1,difficulty+1);
    reason="Missing or truncated spawn file";
    if (!blaze_path(path,sizeof(path),root,relative) || !blaze_file_read(path,&bytes,&size) || size!=2560)
        goto fail;
    for (i=0;i<128;++i) {
        const uint8_t *p=bytes+i*20;
        next.spawns[i].kind=blaze_le32(p);
        next.spawns[i].parameter=blaze_le32(p+4);
        next.spawns[i].flags=blaze_le32(p+8);
        next.spawns[i].x=blaze_le32(p+12)<<5;
        next.spawns[i].y=blaze_le32(p+16)<<5;
    }
    free(bytes); blaze_level_close(out); *out=next; return 1;
fail:
    snprintf(error,capacity,"%s",reason);
resource_fail:
    free(bytes); blaze_level_close(&next); return 0;
}
