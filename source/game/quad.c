#include "blaze.h"

/* 0x41f720/0x41f050: four affine triangles meet at the averaged center.
   Fixed-point edge and paired-pixel rounding follow the original MMX-capable
   renderer, expressed directly as integer interpolation, without SIMD state. */
typedef struct {int64_t x,y,u,v;} Vertex;
static int64_t down(int64_t n) {return n/65536-(n%65536<0);}
static void span(BlazeFrame *frame,const BlazeArchive *archive,const BlazeSprite *s,
    int y,int x,int right,int64_t u,int64_t v,int64_t ru,int64_t rv) {
    int n,i;int64_t du,dv;const uint8_t *pixels=archive->expanded+s->expanded;
    if(right<x) {int a=x;int64_t b=u,c=v;x=right;right=a;u=ru;v=rv;ru=b;rv=c;}
    if(x>=320 || right<0)return;n=right-x+1;
    if(right>=320) {int cut=right-319;ru-=(ru-u)/n*cut;rv-=(rv-v)/n*cut;right=319;n=right-x+1;}
    if(x<0) {u-=(u-ru)/n*(-x);v-=(v-rv)/n*(-x);x=0;n=right+1;}
    if(n<2)return;
    du=(ru-u)*2/(n-1);dv=(v-rv)*2/(n-1);v=(int64_t)s->height*65536-v;
    for(i=0;i<n;++i) {
        int64_t sx=down(u+(i/2)*du+(i&1)*(du/2));
        int64_t sy=(int64_t)s->height-1-down(v+(i/2)*dv+(i&1)*(dv/2));
        if(sx>=0 && sx<s->width && sy>=0 && sy<s->height) {
            uint8_t pixel=pixels[(size_t)sy*s->width+(size_t)sx];
            if(s->transparent<0 || pixel!=(uint8_t)s->transparent)frame->pixels[(size_t)y*320+(size_t)(x+i)]=pixel;
        }
    }
}
static void triangle(BlazeFrame *frame,const BlazeArchive *archive,const BlazeSprite *s,Vertex a,Vertex b,Vertex c) {
    Vertex swap;int64_t height,upper,lower,lx,lu,lv,sx,su,sv,ldx,ldu,ldv,dx,du,dv,y;
    if(b.y<a.y) {swap=a;a=b;b=swap;}if(c.y<b.y) {swap=b;b=c;c=swap;}if(b.y<a.y) {swap=a;a=b;b=swap;}
    if(c.y<0 || a.y>=240 || (a.x<0 && b.x<0 && c.x<0) || (a.x>=320 && b.x>=320 && c.x>=320))return;
    height=c.y-a.y+1;upper=b.y-a.y;lower=c.y-b.y+1;
    lx=sx=a.x*65536;lu=su=a.u;lv=sv=a.v;
    ldx=(c.x-a.x)*65536/height;ldu=(c.u-a.u)/height;ldv=(c.v-a.v)/height;
    if(upper) {
        dx=(b.x-a.x)*65536/upper;du=(b.u-a.u)/upper;dv=(b.v-a.v)/upper;
        for(y=a.y;y<b.y && y<240;++y) {
            if(y>=0)span(frame,archive,s,(int)y,(int)down(sx),(int)down(lx),su,sv,lu,lv);
            sx+=dx;su+=du;sv+=dv;lx+=ldx;lu+=ldu;lv+=ldv;
        }
    } else {sx=b.x*65536;su=b.u;sv=b.v;}
    dx=(c.x-b.x)*65536/lower;du=(c.u-b.u)/lower;dv=(c.v-b.v)/lower;
    for(y=b.y;y<=c.y && y<240;++y) {
        if(y>=0)span(frame,archive,s,(int)y,(int)down(sx),(int)down(lx),su,sv,lu,lv);
        sx+=dx;su+=du;sv+=dv;lx+=ldx;lu+=ldu;lv+=ldv;
    }
}
void blaze_draw_quad(BlazeFrame *frame,const BlazeArchive *archive,size_t index,const int32_t p[8]) {
    const BlazeSprite *s;Vertex v[4],center;unsigned i;int64_t u,vmax;
    if(index>=archive->count || !archive->expanded)return;s=&archive->sprites[index];
    u=(int64_t)s->width*65536-4096;vmax=(int64_t)s->visible_height*65536-4096;
    center.x=((int64_t)p[0]+p[2]+p[4]+p[6])/4;center.y=((int64_t)p[1]+p[3]+p[5]+p[7])/4;center.u=u/2;center.v=vmax/2;
    for(i=0;i<4;++i) {v[i].x=p[i*2];v[i].y=p[i*2+1];v[i].u=i&1?u:4096;v[i].v=i&2?vmax:4096;}
    triangle(frame,archive,s,v[0],v[1],center);triangle(frame,archive,s,v[1],v[3],center);
    triangle(frame,archive,s,v[3],v[2],center);triangle(frame,archive,s,v[0],v[2],center);
}
