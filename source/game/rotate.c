#include "blaze.h"

/* 0x4284a0/0x429ea4: polygon edges and inverse texture coordinates. The
   quarter wave is the original initialization rounded to 18 fractional bits;
   no floating point environment or instruction execution is needed at runtime. */
static const int32_t quarter[65]={
    0,6433,12863,19285,25695,32089,38465,44817,51142,57436,63696,69917,76096,
    82230,88314,94344,100318,106232,112081,117863,123574,129210,134769,140246,
    145639,150945,156159,161279,166302,171225,176045,180759,185364,189857,
    194236,198498,202640,206660,210556,214325,217965,221473,224848,228088,
    231190,234153,236975,239655,242189,244578,246820,248913,250856,252648,
    254288,255774,257107,258285,259307,260173,260882,261434,261828,262065,262144
};
static int64_t sine(unsigned a) {
    int64_t v=quarter[(a&64)?64-(a&63):(a&63)];
    return (a&128)?-v:v;
}
static int64_t minimum(int64_t a,int64_t b) {return a<b?a:b;}
static int64_t maximum(int64_t a,int64_t b) {return a>b?a:b;}
/* Arithmetic shift and Win32 MulDiv rounding expressed without signed shifts. */
static int64_t floor_div(int64_t a,int64_t b) {return a/b-(a%b<0);}
static int64_t muldiv(int64_t a,int64_t b,int64_t d) {
    int64_t n=a*b;int negative=(n<0)!=(d<0);
    if(n<0)n=-n;
    if(d<0)d=-d;
    return negative?-((n+d/2)/d):(n+d/2)/d;
}
void blaze_draw_rotated(BlazeFrame *frame,const BlazeArchive *archive,size_t index,
                        int x,int y,int pivot_x,int pivot_y,unsigned angle,int scale) {
    const BlazeSprite *s;const uint8_t *pixels;
    int64_t cosine,negative_sine,dxw,dyw,dxh,dyh,x0,y0,top,bottom;
    int64_t edge_a,edge_b,width_a,width_b,slope_a,slope_b,step_x,step_y,u0,v0;
    int row;
    if(index>=archive->count || scale<=0)return;
    s=&archive->sprites[index];pixels=archive->expanded+s->expanded;
    cosine=sine(angle+64);negative_sine=-sine(angle);
    dxw=s->width*cosine/scale;dyw=-s->width*negative_sine/scale;
    dxh=s->height*negative_sine/scale;dyh=s->height*cosine/scale;
    x0=(int64_t)x*1024-(negative_sine*pivot_y+cosine*pivot_x)/scale;
    y0=(int64_t)y*1024+(negative_sine*pivot_x-cosine*pivot_y)/scale;
    /* Cull before inverse products, including extreme offscreen coordinates.
       Two pixels of slack preserve the original edge-rounding envelope. */
    if(maximum(maximum(x0,x0+dxw),maximum(x0+dxh,x0+dxw+dxh)) < -2048 ||
       minimum(minimum(x0,x0+dxw),minimum(x0+dxh,x0+dxw+dxh)) > (BLAZE_WIDTH+2)*1024)return;
    top=minimum(minimum(y0+dyw+dyh,y0+dyh),y0+dyw);
    top=maximum(minimum(top+1,y0),0);
    top=floor_div(top+1023,1024)*1024;
    bottom=maximum(maximum(y0+dyw+dyh,y0+dyh),y0+dyw);
    bottom=minimum(floor_div(maximum(bottom-1,y0),1024),BLAZE_HEIGHT-1);
    if(top/1024>bottom)return;
    edge_b=x0;
    if(!dyw) {slope_a=slope_b=0;edge_a=x0;width_a=width_b=dxw;}
    else if(!dyh) {slope_a=slope_b=0;edge_a=x0;width_a=width_b=dxh;}
    else {
        slope_a=muldiv(cosine,1024,-negative_sine);
        edge_a=x0+muldiv(cosine,top-y0,-negative_sine);
        width_a=dxh+muldiv(cosine,-dyh,-negative_sine);
        slope_b=muldiv(negative_sine,1024,cosine);
        edge_b+=muldiv(negative_sine,top-y0,cosine);
        width_b=dxw+muldiv(negative_sine,-dyw,cosine);
    }
    if(width_a<0) {edge_a+=width_a;width_a=-width_a;}
    if(width_b<0) {edge_b+=width_b;width_b=-width_b;}
    step_x=muldiv(cosine,scale,256);step_y=muldiv(-negative_sine,scale,256);
    u0=muldiv(step_y,top-(int64_t)y*1024,1024)-step_x*x+(int64_t)pivot_x*262144;
    v0=muldiv(step_x,top-(int64_t)y*1024,1024)+step_y*x+(int64_t)pivot_y*262144;
    for(row=(int)(top/1024);row<=bottom;++row) {
        int64_t left=minimum(maximum(floor_div(maximum(edge_a,edge_b)+2047,1024),0),BLAZE_WIDTH);
        int64_t right=minimum(floor_div(minimum(edge_a+width_a,edge_b+width_b),1024),BLAZE_WIDTH);
        int col;
        for(col=(int)left;col<right;++col) {
            int64_t sx=floor_div(step_x*col+u0,262144);
            int64_t sy=floor_div(v0-step_y*col,262144);
            /* Edge rounding can reach the last texel boundary. Never form an
               invalid pointer, including on malformed/unusual asset geometry. */
            if(sx>=0 && sx<s->width && sy>=0 && sy<s->height) {
                uint8_t pixel=pixels[(size_t)sy*s->width+(size_t)sx];
                if(s->transparent<0 || pixel!=(uint8_t)s->transparent)
                    frame->pixels[row*BLAZE_WIDTH+col]=pixel;
            }
        }
        u0+=step_y;v0+=step_x;edge_a+=slope_a;edge_b+=slope_b;
    }
}
