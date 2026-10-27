#include "game.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace bloons;
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
std::size_t find(Game& g,int type) {
    for(std::size_t i=0;i<g.balloons.size();++i) if(g.balloons[i].type==type) return i;
    throw std::runtime_error("Missing fixture balloon");
}
int main() {
    try {
        require(swf::frameRate==50&&frameMilliseconds==20,"Original SWF cadence must be 50 FPS");
        require(swf::playareaX==25&&swf::playareaY==0,"Original playarea placement");
        for(int n=1;n<=50;++n) {
            Game g; g.start(n); int count=0,monkeys=0;
            for(auto& col:levels()[n-1].cells) for(int type:col) { if(type==4) ++monkeys; else if(type>0&&type<20) ++count; }
            require(monkeys==1&&g.total==count,"Original level counts changed");
            require(g.total>=g.level->target,"Impossible original target");
            for(int t=0;t<300;++t) g.tick({{600,0}});
            require(g.active,"Idle level ended without taking a shot");
        }
        Game g; g.start(1);
        require(g.monkey.x==125&&g.monkey.y==360,"Original monkey registration position");
        require(std::string(g.level->title)=="Pop Me"&&g.level->target==32&&g.level->darts==5,"Level 1 metadata");
        g.press(); g.tick({{500,50}});
        require(g.power==18,"Initial power clamp");
        for(int i=0;i<100;++i) g.tick({{500,50}});
        require(g.power>=18&&g.power<=45,"Power oscillation bounds");
        g.release(); require(g.darts[0].launched,"Dart launch");
        for(int i=0;i<1000&&g.darts[0].launched;++i) g.tick({{500,50}});
        require(!g.darts[0].launched&&g.shots==1,"Shot charged when dart finishes");
        g.start(1); auto i=find(g,1); g.balloons[i].frozen=true; g.pop(i);
        require(g.popped==0&&!g.balloons[i].popped,"Frozen balloon popped");
        g.balloons[i].frozen=false; g.pop(i); g.pop(i);
        require(g.popped==1,"Duplicate pop counted twice");
        auto fixture=[&](std::initializer_list<int> types) {
            g.start(1); g.balloons.clear();
            for(int type:types) {
                Balloon b; b.type=type; b.position={200,200};
                b.solid=type==22||type==23; g.balloons.push_back(b);
            }
        };
        fixture({2}); g.pop(0); require(g.tacks.size()==8,"Tack burst must create eight tacks");
        fixture({3}); g.pop(0); require(g.shots==-1,"Extra dart must refund one shot");
        fixture({8}); g.pop(0); g.press();
        require(g.darts[0].boomerang&&!g.gotBoomerang&&g.shots==-1,"Boomerang is consumed at aim start");
        fixture({9}); g.pop(0); g.press(); g.tick({{500,100}}); g.release();
        require(g.darts[0].launched&&g.darts[1].launched&&g.darts[2].launched&&!g.gotMultishot,"Multishot launches three darts");
        fixture({10}); g.pop(0); require(g.spikeys.size()==1,"Spikey power-up");
        fixture({6,1,6,22}); g.pop(0);
        require(g.balloons[1].frozen&&!g.balloons[2].frozen&&g.balloons[3].frozen,"Ice radius/type exclusions");
        fixture({5,3,21,22,23}); g.pop(0);
        require(g.popped==2&&g.shots==0,"Bomb suppresses neighboring extra-dart effect");
        require(g.balloons[2].popped&&!g.balloons[3].popped&&!g.balloons[4].popped,"Bomb destroys wood but preserves metal/rubber");
        require(g.balloons[1].type==1&&g.balloons[1].character()==130,"Bomb must not change the neighboring MovieClip's artwork");
        fixture({1}); g.sounds.clear(); g.pop(0);
        require(g.sounds.size()==1&&g.sounds[0]==Sound::Pop,"Red balloon sound linkage");
        fixture({6}); g.sounds.clear(); g.pop(0);
        require(g.sounds.back()==Sound::Ice,"Ice activation sound");
        for(int t=0;t<3;++t) g.tick({});
        require(g.sounds.back()==Sound::IcePop,"Ice sprite's delayed frame-101 sound");
        fixture({7}); g.pop(0);
        require(g.pacmanActive,"Pacman activation");
        auto before=g.pacman;
        Input both; both.right=both.left=both.up=true; g.tick(both);
        require(g.pacman.x==before.x+3&&g.pacman.y==before.y,"Pacman right/left/up input priority");
        for(int t=0;t<201;++t) g.tick({});
        require(!g.pacmanActive&&g.shots==1,"Pacman lifetime and dart consumption");
        fixture({1}); g.easy=true; g.pop(0); g.press(); g.tick({{500,100}}); g.release();
        for(int t=0;t<1000&&g.darts[0].launched;++t) g.tick({{500,100}});
        require(g.shots==0,"Unlimited mode must not consume darts");
        g.easy=false;
        g.start(1);
        for(std::size_t b=0;b<g.balloons.size();++b) g.pop(b);
        for(int t=0;t<50;++t) g.tick({});
        require(g.active,"End check must wait until more than 1000ms");
        g.tick({});
        require(g.outcome==Outcome::Won&&g.profile.unlocked==2&&g.levelScore==100,"Win/progression/score");
        g.start(1); g.shots=5;
        for(int t=0;t<51;++t) g.tick({});
        require(g.outcome==Outcome::Lost,"Loss after final dart and delay");
        bool seen[24]{};
        for(int n=1;n<=50;++n) {
            Game effects; effects.start(n);
            for(std::size_t b=0;b<effects.balloons.size();++b) {
                int type=effects.balloons[b].type; seen[type]=true;
                effects.pop(b);
                for(int frame=0;frame<5;++frame) effects.tick({});
            }
        }
        for(int type=1;type<=10;++type) if(type!=4) require(seen[type],"Missing power-up coverage");
        std::cout<<"All 50 levels, launch, frozen collision, power-up chains, scoring and end states passed.\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
