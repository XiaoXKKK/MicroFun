#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include "TileIndex.hpp"
#include "ViewportAssembler.hpp"
#include "stb_image.h"

int main(int argc, char** argv){
    std::string resourceDir="data/tiles"; int px=0, py=0, sw=128, sh=128; 
    bool outputPNG=false; std::string outFile=""; // backward compat if -o specified
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="-i" && i+1<argc) resourceDir=argv[++i];
        else if(a=="-p" && i+1<argc){ std::string v=argv[++i]; auto c=v.find(','); if(c!=std::string::npos){ px=std::stoi(v.substr(0,c)); py=std::stoi(v.substr(c+1)); }}
        else if(a=="-s" && i+1<argc){ std::string v=argv[++i]; auto x=v.find(','); if(x!=std::string::npos){ sw=std::stoi(v.substr(0,x)); sh=std::stoi(v.substr(x+1)); } else { // fallback accept WxH
                x=v.find('x'); if(x!=std::string::npos){ sw=std::stoi(v.substr(0,x)); sh=std::stoi(v.substr(x+1)); }
            }}
        else if(a=="-o" && i+1<argc){ outputPNG=true; outFile=argv[++i]; }
        else if(a=="-h"){
            std::cout << "Usage: check_tool -i <resource_dir> -p posx,posy -s w,h\n";
            return 0;
        }
    }
    std::string meta = resourceDir + "/meta.txt";
    TileIndex index; if(!index.load(meta)){ std::cerr<<"Failed load meta: "<<meta<<"\n"; return 1; }
    // 输入 posy 以左下角为 0，需要转换为内部使用的“原图顶左为(0,0)”坐标。
    int internalY = index.getMapHeight() - py - sh; // bottom-origin -> top-origin
    if(internalY < 0) internalY = 0; // clamp
    ViewportAssembler assembler; Viewport vp{px,internalY,sw,sh};
    if(outputPNG){
        std::string png = outFile.empty()? (resourceDir+"/viewport.png"): outFile;
        if(!assembler.assemble(index, vp, png)){ std::cerr<<"Assemble failed\n"; return 2; }
        std::cout << "Assemble OK -> "<< png << "\n"; return 0;    
    }
    // hex dump pathway: assemble to memory (modify assembler? we reuse logic by writing tmp file then reading; here implement lightweight duplication)
    auto tiles = index.query(vp);
    if(tiles.empty()){ std::cerr<<"No tiles overlap viewport\n"; return 0; }
    std::vector<unsigned char> canvas(vp.w * vp.h * 4, 0);
    auto blit = [&](const unsigned char *src, int sw_, int sh_, int stride, int dx, int dy){
        for(int y=0;y<sh_;++y){ if(dy+y<0||dy+y>=vp.h) continue; unsigned char* dstRow=&canvas[(dy+y)*vp.w*4]; const unsigned char* srcRow=src+y*stride; for(int x=0;x<sw_;++x){ if(dx+x<0||dx+x>=vp.w) continue; const unsigned char* sp=&srcRow[x*4]; unsigned char* dp=&dstRow[(dx+x)*4]; float a=sp[3]/255.f; for(int c=0;c<3;++c) dp[c]=static_cast<unsigned char>(sp[c]*a + dp[c]*(1-a)); dp[3]=static_cast<unsigned char>(std::min(255.f, sp[3]+ dp[3]*(1-a))); }}
    };
    for(auto &t: tiles){ int w,h,c; std::string tilePath = resourceDir + "/" + t.file; unsigned char* data = stbi_load(tilePath.c_str(), &w,&h,&c,4); if(!data){ std::cerr<<"Fail tile "<<t.file<<"\n"; continue;} int lx=t.x - vp.x; int ly=t.y - vp.y; blit(data,w,h,w*4,lx,ly); stbi_image_free(data);}    
    // output hex values
    std::ios oldState(nullptr); oldState.copyfmt(std::cout);
    std::cout << std::hex << std::uppercase << std::setfill('0');
    size_t count = vp.w * vp.h;
    for(size_t i=0;i<count;++i){
        unsigned char r=canvas[i*4+0]; unsigned char g=canvas[i*4+1]; unsigned char b=canvas[i*4+2]; unsigned char a=canvas[i*4+3];
        uint32_t v=(r<<24)|(g<<16)|(b<<8)|a; // 0xRRGGBBAA
        std::cout << "0x" << std::setw(8) << v; if(i+1<count) std::cout << ",";    }
    std::cout << std::endl;
    std::cout.copyfmt(oldState);
    return 0; 
}
