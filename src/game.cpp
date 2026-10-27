#include "game.hpp"
#include "core_math.hpp"
#ifndef BLOONS_EMBEDDED
#include <stdexcept>
#endif

namespace bloons {
namespace {
double distance2(Vec a, Vec b) { double x=a.x-b.x, y=a.y-b.y; return x*x+y*y; }
double length(Vec v) { return math::sqrt(v.x*v.x+v.y*v.y); }
constexpr double degrees = 57.29577951308232;
bool intersects(Rect a,Rect b) { return a.x<=b.x+b.w&&a.x+a.w>=b.x&&a.y<=b.y+b.h&&a.y+a.h>=b.y; }
Rect hitBounds(const Projectile& p,swf::Bounds box,swf::Affine local,double sx=1,double sy=1) {
    double angle=p.rotation/degrees, c=math::cos(angle),s=math::sin(angle);
    double left=1e30,top=1e30,right=-1e30,bottom=-1e30;
    for(int xi=0;xi<2;++xi) for(int yi=0;yi<2;++yi) {
        double x=box.x+xi*box.w,y=box.y+yi*box.h;
        double px=(local.a*x+local.c*y+local.tx)*sx;
        double py=(local.b*x+local.d*y+local.ty)*sy;
        double wx=p.position.x+c*px-s*py, wy=p.position.y+s*px+c*py;
        left=minimum(left,wx); right=maximum(right,wx); top=minimum(top,wy); bottom=maximum(bottom,wy);
    }
    return {left,top,right-left,bottom-top};
}
}
bool Rect::contains(Vec p) const { return p.x>=x && p.y>=y && p.x<=x+w && p.y<=y+h; }
Rect Balloon::bounds() const {
    if(auto style=swf::balloonStyle(character())) {
        auto r=style->poses[animation%80].hit;
        return {position.x+r.x,position.y+r.y,r.w,r.h};
    }
    // Blocks' collision bounds are their resting frames, not the union of their
    // break/bounce animation. Metal and rubber have half-pixel stroke extents.
    if(type==21) return {position.x,position.y,25,36};
    if(type==22) return {position.x-.5,position.y-.45,26,36.95};
    return {position.x-.30429,position.y-.67045,25.90858,36.97045};
}
Game::Game(uint32_t seed):randomState(seed ? seed : 1) {}
int Game::random(int range) { randomState=randomState*1664525u+1013904223u; return int((randomState>>8)%range); }
void Game::resetDart(Projectile& d) {
    bool extra=d.extra, boom=d.boomerang;
    d=Projectile{}; d.extra=extra; d.boomerang=boom;
    d.position={monkey.x+15,monkey.y+15};
}
void Game::start(int number) {
    if(number<1 || number>50) {
#ifdef BLOONS_EMBEDDED
        platformFailure("Level must be 1..50");
#else
        throw std::out_of_range("Level must be 1..50");
#endif
    }
    level=&levels()[number-1];
    balloons.clear(); tacks.clear(); spikeys.clear(); sounds.clear();
    sounds.push_back(Sound::StopAll);
    popped=total=shots=pacmanAge=throwFrame=0;
    power=sincePop=shake=levelScore=0; powerDirection=1;
    gotBoomerang=gotMultishot=pacmanActive=false; pacman={2000,2000};
    outcome=Outcome::Playing; active=true;
    for(int x=0;x<columns;++x) for(int y=0;y<rows;++y) {
        int type=level->cells[x][y]; if(!type) continue;
        Vec p{swf::playareaX+(x+(type<=20))*25,swf::playareaY+(y+(type<=20))*36};
        if(type==4) { monkey=p; continue; }
        Balloon b; b.position=p; b.type=type; b.solid=type==22||type==23;
        b.colour=type==1 ? random(4) : 0;
        b.symbol=swf::balloonSymbol(type,b.colour);
        b.animation=type==10 ? 20 : random(80);
        if(type==23) {
            auto solid=[&](int a,int c) { return a>=0&&a<columns&&c>=0&&c<rows&&(level->cells[a][c]==22||level->cells[a][c]==23); };
            b.disabled={solid(x-1,y),solid(x+1,y),solid(x,y-1),solid(x,y+1)};
        }
        if(type<20) ++total;
        balloons.push_back(b);
    }
    for(int i=0;i<3;++i) { darts[i]=Projectile{}; darts[i].extra=i!=0; resetDart(darts[i]); }
    aimTip={monkey.x+15,monkey.y+15};
}
bool Game::contact(const Balloon& b,Vec p,double radius) const {
    Rect r=b.bounds(); r.x-=radius; r.y-=radius; r.w+=2*radius; r.h+=2*radius;
    return r.contains(p);
}
bool Game::clipPop(Balloon& b) {
    if(b.frozen||b.solid||b.popped) return false;
    b.symbol=b.character(); b.popped=true; b.popFrame=0;
    if(b.type==21) sounds.push_back(Sound::Wood);
    else if(auto style=swf::balloonStyle(b.character()); style&&style->popSound)
        sounds.push_back(static_cast<Sound>(style->popSound));
    return true;
}
void Game::pop(size_t i) {
    if(i>=balloons.size()||balloons[i].popped) return;
    auto& b=balloons[i]; clipPop(b);
    if(b.frozen||b.solid) return;
    if(b.type==21) return; // wooden block breaks but is not part of the target
    effect(i);
}
void Game::effect(size_t i) {
    auto& b=balloons[i]; if(b.solid||b.type==4||b.type==23) return;
    ++popped; sincePop=0;
    Vec p=b.position;
    switch(b.type) {
    case 2: {
        constexpr double directions[8][2]={{0,-1},{.707,-.707},{1,0},{.707,.707},{0,1},{-.707,.707},{-1,0},{-.707,-.707}};
        for(auto& v:directions) { Projectile t; t.position={p.x-12.5,p.y-18}; t.velocity={v[0]*7,v[1]*7}; tacks.push_back(t); }
        break;
    }
    case 3: --shots; break;
    case 5:
        if(distance2(pacman,p)<10000) deactivatePacman();
        shake=10;
        for(size_t j=0;j<balloons.size();++j) {
            auto& other=balloons[j];
            if(other.popped||other.type==22||other.type==23||distance2(other.position,p)>=10000) continue;
            other.frozen=other.solid=false; clipPop(other);
            if(other.type==21) continue;
            if(other.type!=5 && other.type!=10) other.type=1;
            effect(j);
        }
        sounds.push_back(Sound::StopAll); sounds.push_back(Sound::Bomb); break;
    case 6:
        if(distance2(pacman,p)<10000) deactivatePacman();
        for(auto& other:balloons) if(!other.popped&&other.type!=6&&distance2(other.position,p)<10000) other.frozen=true;
        sounds.push_back(Sound::StopAll); sounds.push_back(Sound::Ice); break;
    case 7:
        pacman={p.x-8.5,p.y-8.5}; pacmanActive=true; pacmanAge=0;
        // attachSound("pacsong") resolves no symbol in this SWF and is silent.
        break;
    case 8: gotBoomerang=true; --shots; break;
    case 9: gotMultishot=true; --shots; break;
    case 10: {
        Projectile s; s.position={p.x-12.5,p.y-18}; s.rotation=random(10)-5;
        spikeys.push_back(s); break;
    }
    }
}
void Game::press() {
    if(active&&!pacmanActive&&shots<level->darts+1&&!darts[0].launched) {
        darts[0].boomerang=gotBoomerang; gotBoomerang=false; darts[0].aiming=true;
    }
}
void Game::launch(Projectile& d,Vec target) {
    d.power=d.boomerang ? maximum(9.0,power/2) : maximum(13.0,power/1.5);
    Vec delta{target.x-d.position.x,target.y-d.position.y}; double dist=length(delta);
    if(dist==0) delta={0,-1},dist=1;
    d.velocity={delta.x/dist*d.power,delta.y/dist*d.power};
    d.rotation=aimAngle*degrees;
    d.launched=true; d.aiming=false;
}
void Game::release() {
    auto& d=darts[0];
    if(!active||shots>=level->darts+1||d.launched||!d.aiming) return;
    Vec target{input.mouse.x-10,input.mouse.y-10};
    if(gotMultishot) { gotMultishot=false; launch(darts[1],{target.x,target.y-70}); launch(darts[2],{target.x,target.y+70}); }
    launch(d,target); power=0; throwFrame=1;
}
void Game::oneLessShot() {
    if(easy) { if(popped>=level->target) checkWin(); }
    else ++shots;
}
void Game::checkWin() {
    if(!active) return;
    active=false;
    if(popped<level->target) { outcome=Outcome::Lost; return; }
    levelScore=math::round(double(popped)/total*1000)/10;
    if(easy) score=0;
    else {
        score+=levelScore;
        profile.unlocked=maximum(profile.unlocked,minimum(50,level->number+1));
        profile.best[level->number-1]=maximum(profile.best[level->number-1],levelScore);
    }
    outcome=level->number==50 ? Outcome::Complete : Outcome::Won;
}
void Game::updateDart(Projectile& d) {
    if(!d.launched) return;
    if(!d.boomerang&&(d.position.x>width-20||d.position.x< -50||d.position.y>height+50)) {
        if(!d.extra) oneLessShot(); resetDart(d); return;
    }
    if(d.stuck) {
        d.alpha-=2;
        if(d.alpha<=0) { if(!d.extra) oneLessShot(); resetDart(d); }
        return;
    }
    if(d.boomerang) {
        Vec delta{monkey.x+20-d.position.x,monkey.y+20-d.position.y}; ++d.age;
        if(d.age%20==1||d.age%20==11) sounds.push_back(Sound::Boomerang);
        d.velocity.x+=delta.x*.001; d.velocity.y+=delta.y*.001;
        if(d.age<30) d.velocity.y+=.35;
        else if(d.age>=60&&d.position.y>=monkey.y) {
            double dist=length(delta); if(dist>0) d.velocity={delta.x/dist*d.power/2,delta.y/dist*d.power/2};
        }
        double speed=length(d.velocity); if(speed>20) { d.velocity.x*=20/speed; d.velocity.y*=20/speed; }
    } else d.velocity.y+=.7;
    for(size_t i=0;i<balloons.size();++i) {
        auto& b=balloons[i]; if(b.popped||distance2(b.position,d.position)>10000) continue;
        bool hit=false;
        for(int axis=0;axis<(d.boomerang?1:2);++axis) {
            for(int sample=1;sample<4;++sample) {
                auto scale=d.extra?swf::extraDart:swf::dart;
                double angle=d.rotation/degrees;
                double tipX=swf::dartTip.tx*scale.a, tipY=swf::dartTip.ty*scale.d;
                Vec p{d.position.x+math::cos(angle)*tipX-math::sin(angle)*tipY,
                      d.position.y+math::sin(angle)*tipX+math::cos(angle)*tipY};
                if(d.boomerang) p=d.position;
                if(!d.boomerang) { if(axis==0) p.x+=sample*d.velocity.x/3; else p.y+=sample*d.velocity.y/3; }
                bool collided=d.boomerang ? intersects(b.bounds(),hitBounds(d,swf::boomerangHitBounds,swf::boomerangHit,scale.a,scale.d)) : contact(b,p);
                if(!collided) continue;
                clipPop(b); hit=true;
                if(b.type==21||b.type==22) { d.stuck=true; if(b.type==22) sounds.push_back(Sound::Metal); return; }
                if(b.frozen||b.type==23) {
                    if(d.boomerang) { d.stuck=true; return; }
                    double drag=b.frozen?.9:.95, bounce=b.frozen?.65:.85;
                    d.velocity.x*=drag; d.velocity.y*=drag;
                    (axis==0?d.velocity.x:d.velocity.y)*=-bounce;
                    d.rebounds+=b.frozen?1:.5;
                    if(b.frozen) sounds.push_back(Sound::Clink);
                    else sounds.push_back(Sound::Rubber);
                    if(d.rebounds>12) d.stuck=true;
                }
                break;
            }
        }
        if(hit) {
            if(b.frozen||b.type==23) break;
            effect(i);
            if(b.type==5) { if(!d.extra) oneLessShot(); resetDart(d); }
        }
    }
    if(d.age>20&&distance2(d.position,{monkey.x+15,monkey.y+15})<400) d.stuck=true;
    if(!d.boomerang) d.rotation=math::atan2(d.velocity.y,d.velocity.x)*degrees+90;
    d.position.x+=d.velocity.x; d.position.y+=d.velocity.y;
}
void Game::updateTack(Projectile& t) {
    if(t.dead) return;
    if(t.age>18) { t.dead=true; return; }
    t.position.x+=t.velocity.x; t.position.y+=t.velocity.y; t.velocity.y+=.05; ++t.age;
    t.rotation=math::atan2(t.velocity.y,t.velocity.x)*degrees+90;
    if(t.checks++<4) return;
    t.checks=0;
    for(size_t i=0;i<balloons.size();++i) {
        auto& b=balloons[i];
        if(b.popped||distance2(b.position,t.position)>10000||!intersects(b.bounds(),hitBounds(t,swf::tackHitBounds,swf::tackHit))) continue;
        clipPop(b);
        if(b.type==22) { t.dead=true; return; }
        if(b.type==21||b.frozen||b.solid) { t.age=18; return; }
        effect(i);
    }
}
void Game::updateSpikey(Projectile& s) {
    if(s.dead) return;
    if(s.stuck) { s.alpha-=2; if(s.alpha<=0) s.dead=true; return; }
    s.position.x+=s.velocity.x; s.position.y+=s.velocity.y; s.velocity.y+=.8; ++s.age;
    for(size_t i=0;i<balloons.size();++i) {
        auto& b=balloons[i]; auto rotating=s; rotating.rotation*=s.age;
        if(b.popped||distance2(b.position,s.position)>10000||!intersects(b.bounds(),hitBounds(rotating,swf::spikeyHitBounds,swf::spikeyHit))) continue;
        clipPop(b);
        if(b.type==21) { if(++s.rebounds>5) s.stuck=true; return; }
        if(b.type==22) { s.stuck=true; sounds.push_back(Sound::Metal); return; }
        if(b.frozen||b.type==23) {
            double drag=b.frozen?.9:.95, bounce=b.frozen?.65:.95;
            s.velocity.x*=drag; s.velocity.y*=drag;
            s.rebounds+=b.frozen?1:.5;
            if(s.rebounds>(b.frozen?6:5)) s.stuck=true;
            Rect r=b.bounds();
            // Side strips correspond to the original MovieClip's four hit areas.
            if(s.position.y<r.y+4||s.position.y>r.y+r.h-4) s.velocity.y*=-bounce;
            else s.velocity.x*=-bounce;
            if(b.frozen) sounds.push_back(Sound::Clink);
            return;
        }
        effect(i); if(b.type==5) s.dead=true;
    }
}
void Game::deactivatePacman() {
    if(pacmanActive&&!gotMultishot&&!gotBoomerang) oneLessShot();
    pacmanActive=false; pacman={2000,2000};
}
void Game::updatePacman() {
    if(!pacmanActive) return;
    Vec next=pacman;
    if(input.right) next.x+=3;
    else if(input.left) next.x-=3;
    else if(input.up) next.y-=3;
    else if(input.down) next.y+=3;
    bool blocked=false;
    for(size_t i=0;i<balloons.size();++i) {
        auto& b=balloons[i]; if(b.popped||!contact(b,next)) continue;
        b.frozen=false; clipPop(b);
        if(b.type!=21) effect(i);
        if(b.solid) blocked=true;
    }
    if(pacmanActive&&!blocked) pacman=next;
    if(++pacmanAge>200) deactivatePacman();
}
void Game::tick(const Input& nextInput,double elapsed) {
    if(!level) return;
    input=nextInput;
    for(auto& d:darts) updateDart(d);
    for(size_t i=0;i<tacks.size();++i) updateTack(tacks[i]);
    for(size_t i=0;i<spikeys.size();++i) updateSpikey(spikeys[i]);
    aimAngle=math::atan2(input.mouse.x-monkey.x,monkey.y-input.mouse.y);
    double tipX=swf::armTip.tx*1.5,tipY=swf::armTip.ty*1.5;
    aimTip={monkey.x+swf::monkeyArm.tx+math::cos(aimAngle)*tipX-math::sin(aimAngle)*tipY,
            monkey.y+swf::monkeyArm.ty+math::sin(aimAngle)*tipX+math::cos(aimAngle)*tipY};
    if(darts[0].aiming) {
        power+=.5*powerDirection;
        if(power>45) { power=45; powerDirection=-1; }
        if(power<18) { power=18; powerDirection=1; }
        darts[0].position=aimTip;
        darts[0].rotation=aimAngle*degrees;
    }
    updatePacman(); sincePop+=elapsed;
    if(active&&!pacmanActive&&sincePop>1000&&(shots>=level->darts||popped>=total)) checkWin();
    if(shake!=0) { shake/=-1.1; if(math::abs(shake)<1) shake=0; }
    // CreateBloon uses gotoAndStop(random(80)+1), not a playing idle timeline.
    for(auto& b:balloons) if(b.popFrame>=0) {
        ++b.popFrame;
        if(b.character()==108&&b.popFrame==3) sounds.push_back(Sound::IcePop);
    }
    if(throwFrame>0) {
        if(++throwFrame==3) sounds.push_back(Sound::Throw);
        if(throwFrame>9) throwFrame=0;
    }
}
}
