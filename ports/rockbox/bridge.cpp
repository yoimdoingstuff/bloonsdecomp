#include "bridge.h"
#include "game.hpp"
#include "core_math.hpp"
#ifndef BLOONS_EMBEDDED
#include <new>
#else
void *operator new(size_t,void *p) noexcept { return p; }
namespace bloons {
[[noreturn]] void platformFailure(const char *message) {
    bloons_platform_failure(message);
    for(;;) {} // Host failure callback must not return.
}
}
#endif

struct bloons_state {
    bloons::Game game; bool held=false; int wheel=24;
    explicit bloons_state(uint32_t seed):game(seed) {}
};
using namespace bloons;
namespace {
constexpr double pi=3.14159265358979323846;
struct Transform {
    double a=1,b=0,c=0,d=1,x=0,y=0;
    Transform then(swf::Affine t) const {
        return {a*t.a+c*t.b,b*t.a+d*t.b,a*t.c+c*t.d,b*t.c+d*t.d,
                a*t.tx+c*t.ty+x,b*t.tx+d*t.ty+y};
    }
};
Transform position(Vec p,double rotation=0,double sx=1,double sy=1) {
    double c=math::cos(rotation),s=math::sin(rotation);
    return {c*sx,s*sx,-s*sy,c*sy,p.x,p.y};
}
void sprite(const bloons_painter *p,int id,Transform t,int alpha=255) {
    p->sprite(id,t.a,t.b,t.c,t.d,t.x,t.y,alpha);
}
void number(char *out,int value) {
    char digits[12]; int n=0;
    if(value<0) { *out++='-'; value=-value; }
    do { digits[n++]=char('0'+value%10); value/=10; } while(value);
    while(n) *out++=digits[--n]; *out=0;
}
}
extern "C" {
size_t bloons_memory_size(void) { return sizeof(bloons_state)+alignof(bloons_state)-1; }
bloons_state *bloons_create(void *buffer,size_t size,uint32_t seed) {
    uintptr_t start=reinterpret_cast<uintptr_t>(buffer);
    uintptr_t aligned=(start+alignof(bloons_state)-1)&~uintptr_t(alignof(bloons_state)-1);
    if(!buffer||size<aligned-start+sizeof(bloons_state)) return nullptr;
    auto state=new(reinterpret_cast<void*>(aligned)) bloons_state(seed);
    return state;
}
void bloons_start(bloons_state *s,int level,int easy) {
    if(!s||level<1||level>50) return;
    if(s->game.level&&s->game.level->number==level) s->game.score-=s->game.levelScore;
    s->game.easy=easy!=0; s->game.start(level); s->held=false;
}
void bloons_tick(bloons_state *s,int wheel,unsigned directions,int held) {
    if(!s||!s->game.level) return;
    if(wheel>=0&&wheel<96) s->wheel=wheel;
    double a=s->wheel*pi/48;
    Input input;
    input.mouse={s->game.monkey.x+math::sin(a)*500,s->game.monkey.y-math::cos(a)*500};
    input.up=directions&1; input.down=directions&2;
    input.left=directions&4; input.right=directions&8;
    s->game.input=input;
    if(held&&!s->held) s->game.press();
    if(!held&&s->held) s->game.release();
    s->held=held!=0;
    s->game.tick(input,frameMilliseconds);
    // Audio is intentionally not replaced by beeps. Mixing original MP3 effects
    // through Rockbox PCM is a remaining hardware task.
    s->game.sounds.clear();
}
void bloons_cancel_fire(bloons_state *s) {
    if(s) { s->held=false; s->game.darts[0].aiming=false; s->game.power=0; }
}
void bloons_status(const bloons_state *s,struct bloons_status *out) {
    if(!s||!out) return;
    const auto& g=s->game;
    *out={g.level?g.level->number:1,g.level?g.level->target:0,g.popped,
          g.level?maximum(0,g.level->darts-g.shots):0,int(g.outcome),g.pacmanActive,
          g.profile.unlocked,g.easy};
}
void bloons_paint(const bloons_state *s,const bloons_painter *p) {
    if(!s||!s->game.level||!p||!p->sprite||!p->text) return;
    const auto& g=s->game;
    for(const auto& b:g.balloons) {
        auto t=position({b.position.x,b.position.y+g.shake});
        if(b.popped) continue;
        if(auto style=swf::balloonStyle(b.character())) {
            auto body=t.then(style->poses[b.animation%80].inner);
            sprite(p,style->body,body);
            if(b.frozen) sprite(p,49,body);
        } else sprite(p,b.character(),t);
    }
    auto monkey=position({g.monkey.x,g.monkey.y+g.shake});
    sprite(p,155,monkey);
    auto arm=monkey.then(swf::monkeyArm).then({math::cos(g.aimAngle),math::sin(g.aimAngle),
                                              -math::sin(g.aimAngle),math::cos(g.aimAngle),0,0});
    if(g.throwFrame) arm=monkey.then(swf::armThrow).then(swf::throwPoses[minimum(8,g.throwFrame-1)]);
    sprite(p,152,arm);
    for(const auto& t:g.tacks) if(!t.dead) sprite(p,12,position(t.position,t.rotation*pi/180));
    for(const auto& t:g.spikeys) if(!t.dead) sprite(p,8,position(t.position,t.rotation*t.age*pi/180),int(t.alpha*2.55));
    for(const auto& d:g.darts) if(d.launched||d.aiming) {
        auto scale=d.extra?swf::extraDart:swf::dart;
        auto t=position(d.position,d.rotation*pi/180,scale.a,scale.d);
        if(d.boomerang) sprite(p,37,t.then(swf::boomerangArt),int(d.alpha*2.55));
        else sprite(p,33,t.then({1,0,0,1,-55.8,-251.65}),int(d.alpha*2.55));
    }
    if(g.pacmanActive) sprite(p,163,position(g.pacman));
    if(g.active&&!g.darts[0].launched&&!g.pacmanActive) {
        auto arrow=position(g.aimTip,g.aimAngle).then(swf::aimInner);
        if(g.darts[0].aiming) arrow=arrow.then({1,0,0,1+(g.power-18)*.08,0,0});
        sprite(p,170,arrow);
    }
    char value[16]; number(value,g.level->number); p->text(3,2,value);
    p->text(18,2,g.level->title);
    number(value,g.popped); p->text(4,226,value); p->text(28,226,"popped");
    number(value,g.level->target); p->text(80,226,value); p->text(106,226,"target");
    number(value,maximum(0,g.level->darts-g.shots)); p->text(160,226,value); p->text(184,226,"darts");
    if(g.gotBoomerang) sprite(p,275,position({470,440}));
    if(g.gotMultishot) sprite(p,271,position({520,440}));
    if(g.outcome==Outcome::Lost) p->text(118,112,"TRY AGAIN");
    if(g.outcome==Outcome::Won) p->text(92,112,"LEVEL COMPLETE");
    if(g.outcome==Outcome::Complete) p->text(92,112,"GAME COMPLETED");
}
void bloons_profile_get(const bloons_state *s,unsigned char out[104]) {
    out[0]='B'; out[1]='L'; out[2]=1; out[3]=(unsigned char)s->game.profile.unlocked;
    for(int i=0;i<50;++i) {
        unsigned v=(unsigned)math::round(s->game.profile.best[i]*10);
        out[4+i*2]=(unsigned char)v; out[5+i*2]=(unsigned char)(v>>8);
    }
}
int bloons_profile_set(bloons_state *s,const unsigned char in[104]) {
    if(!s||in[0]!='B'||in[1]!='L'||in[2]!=1||in[3]<1||in[3]>50) return 0;
    Profile profile; profile.unlocked=in[3];
    for(int i=0;i<50;++i) {
        unsigned v=in[4+i*2]|(unsigned(in[5+i*2])<<8); if(v>1000) return 0;
        profile.best[i]=double(v)/10;
    }
    s->game.profile=profile; return 1;
}
}
