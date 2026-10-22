#include "blaze.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint16_t blaze_le16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1]<<8); }
uint32_t blaze_le32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static int signed16(uint16_t x) { return x <= INT16_MAX ? (int)x : (int)x - 65536; }
int blaze_path(char *out, size_t capacity, const char *root, const char *relative) {
    int n = snprintf(out, capacity, "%s/%s", root, relative);
    return n >= 0 && (size_t)n < capacity;
}
int blaze_file_read(const char *path, uint8_t **bytes, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *data;
    long length;
    *bytes = NULL; *size = 0;
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 ||
        length > 64*1024*1024 || fseek(file, 0, SEEK_SET)) { fclose(file); return 0; }
    data = malloc(length ? (size_t)length : 1);
    if (!data) { fclose(file); return 0; }
    if (fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data); fclose(file); return 0;
    }
    fclose(file); *bytes = data; *size = (size_t)length; return 1;
}
void blaze_archive_free(BlazeArchive *a) {
    free(a->bytes); free(a->expanded); free(a->sprites); memset(a, 0, sizeof(*a));
}
/* 0x424a40 plus bounds validation. Header length excludes its own v1 word.
   Each row is a sequence of u16 skip/u16 count, literals, DWORD padding.
   Rows are stored bottom-up, including header crop rows. */
int blaze_archive_decode(BlazeArchive *out, const uint8_t *bytes, size_t size,
                         char *error, size_t capacity) {
    BlazeArchive a = {0};
    size_t offset = 0x40c, i, expanded_size=0;
    const char *reason = "invalid DAR header";
    if (size < offset || memcmp(bytes, "DAR:8", 5) || bytes[5] > 1) goto fail;
    a.count = blaze_le16(bytes+6);
    if (!a.count || a.count > 2048) goto fail;
    a.bytes = malloc(size); a.sprites = calloc(a.count, sizeof(*a.sprites));
    if (!a.bytes || !a.sprites) { reason = "DAR allocation failed"; goto fail; }
    memcpy(a.bytes, bytes, size); a.size = size;
    for (i=0; i<a.count; ++i) {
        size_t header = 8, payload, row;
        BlazeSprite *s = &a.sprites[i];
        reason = "truncated DAR record";
        if (bytes[5]) {
            if (offset > size || size-offset < 2) goto fail;
            header = blaze_le16(bytes+offset); offset += 2;
        }
        if (header < 8 || offset > size || header > size-offset) goto fail;
        s->width = blaze_le16(bytes+offset); s->height = blaze_le16(bytes+offset+2);
        s->stride = blaze_le16(bytes+offset+4);
        s->transparent = (int16_t)signed16(blaze_le16(bytes+offset+6));
        s->visible_height = s->height;
        if (header > 13) {
            s->origin_x = (int16_t)signed16(blaze_le16(bytes+offset+8));
            s->origin_y = (int16_t)signed16(blaze_le16(bytes+offset+10));
            s->visible_height = blaze_le16(bytes+offset+12);
        }
        payload = (size_t)s->height * s->stride;
        offset += header; s->pixels = offset;
        if (!s->width || !s->height || s->width > 8192 || s->height > 8192 ||
            !s->stride || (s->stride & 3) || (offset & 3) ||
            s->visible_height > s->height || payload > size-offset) goto fail;
        reason = "invalid DAR row runs";
        for (row=0; row<s->height; ++row) {
            size_t p=offset+row*s->stride, end=p+s->stride, x=0;
            while (x < s->width) {
                size_t skip, count;
                if (end-p < 4) goto fail;
                skip=blaze_le16(bytes+p); count=blaze_le16(bytes+p+2); p+=4;
                if (!skip && !count) goto fail;
                if (skip > s->width-x) goto fail;
                x+=skip;
                if (count > s->width-x || count > end-p) goto fail;
                x+=count; p=(p+count+3)&~(size_t)3;
                if (p>end) goto fail;
            }
        }
        offset += payload;
        s->expanded=expanded_size;
        if ((size_t)s->width*s->height > 64*1024*1024-expanded_size) goto fail;
        expanded_size+=(size_t)s->width*s->height;
    }
    a.expanded=malloc(expanded_size);
    if(!a.expanded) {reason="DAR pixel allocation failed";goto fail;}
    for(i=0;i<a.count;++i) {
        const BlazeSprite *s=&a.sprites[i];size_t row;
        memset(a.expanded+s->expanded,(uint8_t)s->transparent,(size_t)s->width*s->height);
        for(row=0;row<s->height;++row) {
            size_t p=s->pixels+row*s->stride,x=0;
            uint8_t *dest=a.expanded+s->expanded+(s->height-1-row)*s->width;
            while(x<s->width) {
                size_t count=blaze_le16(a.bytes+p+2);
                x+=blaze_le16(a.bytes+p);p+=4;
                memcpy(dest+x,a.bytes+p,count);x+=count;p=(p+count+3)&~(size_t)3;
            }
        }
    }
    blaze_archive_free(out); *out=a; return 1;
fail:
    snprintf(error, capacity, "%s", reason); blaze_archive_free(&a); return 0;
}
int blaze_archive_read(BlazeArchive *out, const char *path, char *error, size_t capacity) {
    uint8_t *bytes; size_t size; int ok;
    if (!blaze_file_read(path, &bytes, &size)) {
        snprintf(error, capacity, "Cannot read %s", path); return 0;
    }
    ok=blaze_archive_decode(out, bytes, size, error, capacity); free(bytes); return ok;
}

/* One immutable rendered-image owner, in the existing bounded span format.
   Capturing is transactional; no game records or executable resources are copied. */
int blaze_frame_capture(BlazeArchive *out,const BlazeFrame *frame) {
    BlazeArchive next={0};unsigned row;
    next.size=240*(4+320);next.count=1;
    next.bytes=malloc(next.size);next.sprites=calloc(1,sizeof(*next.sprites));
    if(!next.bytes || !next.sprites) {blaze_archive_free(&next);return 0;}
    next.sprites[0].width=320;next.sprites[0].height=next.sprites[0].visible_height=240;
    next.sprites[0].stride=324;next.sprites[0].transparent=-1;
    for(row=0;row<240;++row) {
        uint8_t *p=next.bytes+(size_t)row*324;p[0]=p[1]=0;p[2]=64;p[3]=1;
        memcpy(p+4,frame->pixels+(size_t)(239-row)*320,320);
    }
    blaze_archive_free(out);*out=next;return 1;
}
