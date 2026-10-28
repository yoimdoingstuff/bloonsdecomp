#include "bridge.h"
#include "render.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" void bloons_platform_failure(const char *message) { std::fprintf(stderr,"%s\n",message); std::abort(); }
extern "C" void bloons_render_text(int,int,const char *) {}
void require(bool value,const char *message) { if(!value) { std::fprintf(stderr,"%s\n",message); std::exit(1); } }
int main(int argc,char **argv) {
    std::vector<unsigned char> memory(bloons_memory_size());
    require(!bloons_create(memory.data(),1,1),"Undersized core memory accepted");
    auto game=bloons_create(memory.data(),memory.size(),1); require(game,"Core arena alignment");
    for(int level=1;level<=50;++level) {
        bloons_start(game,level,0); struct bloons_status status;
        bloons_status(game,&status); require(status.level==level&&status.outcome==BLOONS_PLAYING,"Original level inaccessible");
        for(int i=0;i<51;++i) bloons_tick(game,24,0,0);
        bloons_status(game,&status); require(status.outcome==BLOONS_PLAYING,"Idle adapter takes shots");
    }
    bloons_start(game,1,0);
    for(int i=0;i<30;++i) bloons_tick(game,24,0,1);
    bloons_tick(game,24,0,0);
    for(int i=0;i<500;++i) bloons_tick(game,24,0,0);
    struct bloons_status status; bloons_status(game,&status);
    require(status.darts==4,"Hold/release must fire exactly one dart");
    unsigned char profile[104]; bloons_profile_get(game,profile);
    profile[3]=19; require(bloons_profile_set(game,profile),"Valid profile rejected");
    bloons_status(game,&status); require(status.unlocked==19,"Unlock persistence");
    profile[5]=255; require(!bloons_profile_set(game,profile),"Corrupt profile accepted");
    bloons_status(game,&status); require(status.unlocked==19,"Corrupt profile partially applied");
    if(argc>1) {
        std::ifstream file(argv[1],std::ios::binary);
        std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)),{});
        require(!data.empty()&&bloons_assets_load(data.data(),data.size()),"Asset pack rejected");
        require(!bloons_assets_load(data.data(),16),"Truncated asset directory accepted");
        require(bloons_assets_load(data.data(),data.size()),"Asset reload failed");
        std::vector<uint16_t> image(320*240+2); image.front()=0x1234; image.back()=0xabcd;
        bloons_render_begin(image.data()+1); bloons_paint(game,bloons_renderer());
        require(image.front()==0x1234&&image.back()==0xabcd,"Renderer wrote outside framebuffer");
        require(image[1]!=0,"Original sky was not drawn");
        if(argc>2) {
            std::ofstream preview(argv[2],std::ios::binary); preview<<"P6\n320 240\n255\n";
            for(size_t i=1;i<=320*240;++i) {
                unsigned char rgb[3]={static_cast<unsigned char>((image[i]>>11)*255/31),
                    static_cast<unsigned char>(((image[i]>>5)&63)*255/63),static_cast<unsigned char>((image[i]&31)*255/31)};
                preview.write(reinterpret_cast<char*>(rgb),3);
            }
        }
    }
    std::printf("Rockbox adapter passed; core arena %zu bytes.\n",memory.size());
}
