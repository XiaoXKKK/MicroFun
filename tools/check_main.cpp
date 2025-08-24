#include <iostream>
#include <string>
#include "TileIndex.hpp"
#include "ViewportAssembler.hpp"

int main(int argc, char** argv){
    std::string meta="data/tiles/meta.txt"; int px=0, py=0, sw=128, sh=128; std::string outFile="data/viewport.png";
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="-i" && i+1<argc) meta=argv[++i];
        else if(a=="-p" && i+1<argc){ std::string v=argv[++i]; auto c=v.find(','); if(c!=std::string::npos){ px=std::stoi(v.substr(0,c)); py=std::stoi(v.substr(c+1)); }}
        else if(a=="-s" && i+1<argc){ std::string v=argv[++i]; auto x=v.find('x'); if(x!=std::string::npos){ sw=std::stoi(v.substr(0,x)); sh=std::stoi(v.substr(x+1)); }}
        else if(a=="-o" && i+1<argc) outFile=argv[++i];
        else if(a=="-h"){
            std::cout << "Usage: check_tool -i <meta_file> -p X,Y -s WxH [-o out_png]\n"; return 0;
        }
    }
    TileIndex index; if(!index.load(meta)){ std::cerr<<"Failed load meta: "<<meta<<"\n"; return 1; }
    ViewportAssembler assembler; Viewport vp{px,py,sw,sh};
    if(!assembler.assemble(index, vp, outFile)){ std::cerr<<"Assemble failed\n"; return 2; }
    std::cout << "Assemble OK -> "<< outFile << "\n"; return 0;
}
