#include "render.h"
static const unsigned char *pack;
static size_t pack_size;
static unsigned count;
static uint16_t *pixels;
static unsigned u16(const unsigned char *p) { return p[0]|((unsigned)p[1]<<8); }
static uint32_t u32(const unsigned char *p) { return u16(p)|((uint32_t)u16(p+2)<<16); }
static float f32(const unsigned char *p) {
    union { uint32_t bits; float value; } v; v.bits=u32(p); return v.value;
}
int bloons_assets_load(const void *data,size_t bytes) {
    const unsigned char *p=data; unsigned i,n;
    if(!p||bytes<12||p[0]!='B'||p[1]!='L'||p[2]!='R'||p[3]!='B'||u32(p+4)!=1) return 0;
    n=u32(p+8); if(n>128||bytes<12+(size_t)n*28) return 0;
    for(i=0;i<n;++i) {
        const unsigned char *r=p+12+i*28;
        size_t offset=u32(r+24),size=(size_t)u16(r+20)*u16(r+22)*4;
        if(!u16(r+20)||!u16(r+22)||offset>bytes||size>bytes-offset||
           !(f32(r+12)>0)||!(f32(r+16)>0)) return 0;
    }
    pack=p; pack_size=bytes; count=n; return 1;
}
static void paint(int id,double a,double b,double c,double d,double tx,double ty,int alpha) {
    const unsigned char *r=0,*source; unsigned i,w,h;
    double x,y,sx,sy,det,left=1e10,top=1e10,right=-1e10,bottom=-1e10;
    int minx,miny,maxx,maxy,px,py;
    if(!pack||!pixels||alpha<=0) return;
    for(i=0;i<count;++i) if(u32(pack+12+i*28)==(unsigned)id) { r=pack+12+i*28; break; }
    if(!r) return;
    x=f32(r+4); y=f32(r+8); sx=f32(r+12); sy=f32(r+16); w=u16(r+20); h=u16(r+22);
    source=pack+u32(r+24); (void)pack_size;
    a*=.5; b*=.5; c*=.5; d*=.5; tx*=.5; ty*=.5;
    det=a*d-b*c; if(det>-1e-12&&det<1e-12) return;
    for(i=0;i<4;++i) {
        double u=x+(i&1)*w*sx,v=y+(i>>1)*h*sy;
        double qx=a*u+c*v+tx,qy=b*u+d*v+ty;
        if(qx<left) left=qx; if(qx>right) right=qx;
        if(qy<top) top=qy; if(qy>bottom) bottom=qy;
    }
    /* Clamp floating bounds before conversion, including fully offscreen clips. */
    if(right<0||bottom<0||left>=320||top>=240) return;
    minx=left<0?0:(int)left; miny=top<0?0:(int)top;
    maxx=right>=319?319:(int)right+1; maxy=bottom>=239?239:(int)bottom+1;
    for(py=miny;py<=maxy;++py) for(px=minx;px<=maxx;++px) {
        double dx=px+.5-tx,dy=py+.5-ty;
        double u=((d*dx-c*dy)/det-x)/sx,v=((-b*dx+a*dy)/det-y)/sy;
        const unsigned char *s; unsigned opacity,red,green,blue; uint16_t old;
        if(u<0||v<0||u>=w||v>=h) continue;
        s=source+((size_t)(unsigned)v*w+(unsigned)u)*4;
        opacity=(unsigned)s[3]*(unsigned)alpha/255; if(!opacity) continue;
        old=pixels[py*320+px];
        red=((old>>11)*255/31*(255-opacity)+s[0]*opacity)/255;
        green=(((old>>5)&63)*255/63*(255-opacity)+s[1]*opacity)/255;
        blue=((old&31)*255/31*(255-opacity)+s[2]*opacity)/255;
        pixels[py*320+px]=(uint16_t)(((red>>3)<<11)|((green>>2)<<5)|(blue>>3));
    }
}
static const struct bloons_painter painter={paint,bloons_render_text};
const struct bloons_painter *bloons_renderer(void) { return &painter; }
void bloons_render_begin(uint16_t *framebuffer) {
    unsigned i; pixels=framebuffer;
    for(i=0;i<320*240;++i) pixels[i]=0;
    paint(10000,1,0,0,1,0,0,255);
}
