#include "blaze.h"
#include <string.h>
#include <stdlib.h>

/* 0x423f50/0x423cf0: legacy palette search, including its strict improvement
   and exact-match early exit. Reserved Windows palette slots are excluded. */
void blaze_color_map(uint8_t out[256],const uint8_t palette[1024],const uint8_t source[1024],unsigned rgb,unsigned percent) {
    unsigned sorted[256]={0},start[257],count=0,r,i;
    for(r=0;r<256;++r) {
        start[r]=count;
        for(i=0;i<256;++i)if(i!=8 && i!=9 && i!=246 && i!=247 && palette[i*4+2]==r)sorted[count++]=i;
    }
    start[256]=count;
    for(i=0;i<256;++i) {
        int target[3],sum=0,extent=0;unsigned c,index,end,best=i;
        for(c=0;c<3;++c) {
            int from=source[i*4+c],to=(rgb>>(c*8))&255,d;
            target[c]=from+(to-from)*(int)percent/100;
            d=abs((int)palette[i*4+c]-target[c]);sum+=d;if(d>extent)extent=d;
        }
        index=start[target[2]>extent?target[2]-extent:0];
        end=start[target[2]+extent<255?target[2]+extent:255];
        while(index<=end && index<count) {
            unsigned candidate=sorted[index];int a=abs(palette[candidate*4]-target[0]);
            int b=abs(palette[candidate*4+1]-target[1]),d=abs(palette[candidate*4+2]-target[2]);
            if(a<=extent && b<=extent) {
                int distance=a+b+d;
                if(!distance)break;
                if(distance<sum) {
                    unsigned lower;sum=distance;best=candidate;
                    extent=a>b?a:b;if(d>extent)extent=d;
                    lower=start[target[2]-extent+1>0?target[2]-extent+1:0];
                    if(index<lower)index=lower-1;
                    end=start[target[2]+extent<256?target[2]+extent:256];
                }
            }
            ++index;
        }
        out[i]=(uint8_t)best;
    }
}

void blaze_color_table(uint8_t out[256],const uint8_t palette[1024],unsigned rgb,unsigned percent) {
    blaze_color_map(out,palette,palette,rgb,percent);
}

/* Forward span scaling and bottom-up row selection from 0x429220. Shrinking
   horizontal spans uses ceil sampling; it is not an inverse nearest blit. */
void blaze_draw_scaled(BlazeFrame *frame,const BlazeArchive *archive,size_t index,
    int x,int y,int scale_x,int scale_y,const uint8_t *rop,const uint8_t *colors) {
    const BlazeSprite *s;int64_t ox,oy,width,height,bottom,top,row,cut,first_source,remainder;
    if(index>=archive->count || scale_x<=0 || scale_y<=0)return;
    s=&archive->sprites[index];
    ox=(int64_t)x-(int64_t)s->origin_x*scale_x/256;
    oy=(int64_t)y-(int64_t)s->origin_y*scale_y/256;
    width=(int64_t)s->width*scale_x/256;height=(int64_t)s->visible_height*scale_y/256;
    bottom=oy+height;if(bottom>240)bottom=240;
    top=oy>0?oy:0;
    if(ox>=320 || ox+width<=0 || top>=bottom)return;
    cut=oy+height-bottom;first_source=cut*256/scale_y;remainder=cut*256%scale_y;
    for(row=0;row<bottom-top;++row) {
        int64_t source_row=first_source+(scale_y<256?row*256/scale_y:
            (((row+1+remainder/256)*256+scale_y-1)/scale_y-1));
        size_t p;int sx=0;int64_t fixed=0,left=ox>0?ox:0,right=ox+width<320?ox+width:320;
        if(source_row<0 || source_row>=s->visible_height)continue;
        p=s->pixels+(size_t)(s->height-s->visible_height+source_row)*s->stride;
        fixed=-(left-ox)*256;
        while(sx<s->width) {
            int skip=blaze_le16(archive->bytes+p),count=blaze_le16(archive->bytes+p+2),i=0,processed=0;
            int64_t dx;fixed+=(int64_t)skip*scale_x;sx+=skip;p+=4;
            dx=fixed/256-(fixed%256<0);
            if(dx>=right-left)break;
            if(scale_x==256) {
                int64_t begin=dx<0?-dx:0;
                for(;begin<count && dx+begin<right-left;++begin) {
                    uint8_t pixel=archive->bytes[p+(size_t)begin],*dest;
                    if(colors)pixel=colors[pixel];
                    dest=&frame->pixels[(size_t)(bottom-1-row)*320+(size_t)(left+dx+begin)];
                    *dest=rop?rop[((size_t)pixel<<8)|*dest]:pixel;
                }
                fixed+=(int64_t)count*256;sx+=count;p=(p+(size_t)count+3)&~(size_t)3;
                continue;
            }
            if(dx<0) {
                while(i<count && fixed < -scale_x) {++i;fixed+=scale_x;}
                dx=0;
            }
            while(i<count && dx<right-left) {
                uint8_t pixel=archive->bytes[p+(size_t)i];int64_t end=dx+1;
                processed=1;
                if(colors)pixel=colors[pixel];
                if(scale_x>256) {fixed+=scale_x;end=fixed/256-(fixed%256<0);if(end>right-left)end=right-left;}
                /* The original 1x..4x fast path writes a word even for a
                   zero-length first repeat after left clipping. Preserve that
                   visible bleed, but never leave the owned framebuffer. */
                if(scale_x>256 && scale_x<=1024 && end==dx && !rop) {
                    size_t pos=(size_t)(bottom-1-row)*320+(size_t)(left+dx),k;
                    for(k=0;k<2 && pos+k<sizeof(frame->pixels);++k)frame->pixels[pos+k]=pixel;
                }
                while(dx<end) {
                    uint8_t *dest=&frame->pixels[(size_t)(bottom-1-row)*320+(size_t)(left+dx++)];
                    *dest=rop?rop[((size_t)pixel<<8)|*dest]:pixel;
                }
                ++i;
                if(scale_x<=256) {
                    fixed+=scale_x;
                    while(i<count && fixed<dx*256) {++i;fixed+=scale_x;}
                }
            }
            /* 0x42ba4d/58 bypass restoration of the span accumulator. With
               the original palette allocation this terminates a tinted
               enlarged row after its first processed run. Encode that visible
               rule directly, without pointer-to-integer arithmetic. */
            if(colors && scale_x>256 && processed)break;
            sx+=count;p=(p+(size_t)count+3)&~(size_t)3;
        }
    }
}

/* 0x419d40/0x429220: the first 32 attachment frames displace each row
   horizontally. Exact quarter-wave bytes from 0x43c3c8, reflected by symmetry;
   clipping restarts the phase at the bottom visible row, as in the original. */
void blaze_draw_wave(BlazeFrame *frame,const BlazeArchive *archive,size_t index,
    int x,int y,unsigned age,const uint8_t *colors) {
    static const int8_t quarter[65]={
        0,3,6,9,12,15,18,21,24,28,31,34,37,40,43,46,
        48,51,54,57,60,63,65,68,71,73,76,78,81,83,85,88,
        90,92,94,96,98,100,102,104,106,108,109,111,112,114,115,117,
        118,119,120,121,122,123,124,124,125,126,126,127,127,127,127,127,127};
    const BlazeSprite *s;int bottom,top,row;
    if(age>=32) {blaze_draw_scaled(frame,archive,index,x,y,256,256,NULL,colors);return;}
    if(index>=archive->count)return;
    s=&archive->sprites[index];x-=s->origin_x;y-=s->origin_y;
    bottom=y+s->visible_height;if(bottom>240)bottom=240;top=y>0?y:0;
    if(x>=320 || x+s->width<=0 || top>=bottom)return;
    for(row=0;row<bottom-top;++row) {
        unsigned phase=(age+(unsigned)row*64)&255,q=phase&127;
        int wave=quarter[q<=64?q:128-q],product,shift,sx=0,dx;
        size_t p=s->pixels+(size_t)(s->height-s->visible_height+y+s->visible_height-bottom+row)*s->stride;
        if(phase&128)wave=-wave;
        product=wave*(32-(int)age);shift=product/128-(product%128<0);dx=x+shift;
        while(sx<s->width) {
            unsigned skip=blaze_le16(archive->bytes+p),count=blaze_le16(archive->bytes+p+2),i;
            p+=4;sx+=(int)skip;
            for(i=0;i<count;++i)if(dx+sx+(int)i>=0 && dx+sx+(int)i<320) {
                uint8_t pixel=archive->bytes[p+i];
                frame->pixels[(size_t)(bottom-1-row)*320+(size_t)(dx+sx+(int)i)]=colors?colors[pixel]:pixel;
            }
            sx+=(int)count;p=(p+count+3)&~(size_t)3;
        }
    }
}

/* Native clipped indexed blit, recovered from 0x427e60/0x428030.
   ROP lookup is (source << 8) | destination. Transparent spans leave the
   destination intact. No host casts, guest registers or writable mirrors. */
void blaze_draw(BlazeFrame *frame, const BlazeArchive *archive, size_t index,
                int x, int y, const uint8_t *rop) {
    const BlazeSprite *s;
    int row;
    if (index >= archive->count) return;
    s=&archive->sprites[index]; x-=s->origin_x; y-=s->origin_y;
    for (row=0; row<s->visible_height; ++row) {
        int dy=y+row, sx=0;
        size_t p=s->pixels+(size_t)(s->height-1-row)*s->stride;
        if (dy<0 || dy>=BLAZE_HEIGHT) continue;
        while (sx<s->width) {
            int skip=blaze_le16(archive->bytes+p), count=blaze_le16(archive->bytes+p+2), i;
            p+=4; sx+=skip;
            for (i=0; i<count; ++i) {
                int dx=x+sx+i;
                if (dx>=0 && dx<BLAZE_WIDTH) {
                    uint8_t *dest=&frame->pixels[dy*BLAZE_WIDTH+dx];
                    uint8_t source=archive->bytes[p+(size_t)i];
                    *dest=rop ? rop[((size_t)source<<8)|*dest] : source;
                }
            }
            sx+=count; p=(p+(size_t)count+3)&~(size_t)3;
        }
    }
}
/* 0x419500: ASCII 32..90, 8-pixel cell; alignment 2=right,4=center. */
void blaze_text(BlazeFrame *frame, const BlazeArchive *font, const char *text,
                int x, int y, int alignment, const uint8_t *rop) {
    size_t i, length=strlen(text);
    int width=font->count ? font->sprites[0].width : 8;
    if (length > 128) return;
    if (alignment==2) x+=BLAZE_WIDTH-(int)length*width;
    if (alignment==4) x+=(BLAZE_WIDTH-(int)length*width)/2;
    for (i=0; i<length; ++i) {
        unsigned c=(unsigned char)text[i];
        if (c>=32 && c<=90) blaze_draw(frame,font,c-32,x+(int)i*width,y,rop);
    }
}
