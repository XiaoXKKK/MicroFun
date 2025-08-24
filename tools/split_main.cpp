#include <iostream>
#include <string>
#include <filesystem>
#include "TileSplitter.hpp"
#include "TileIndex.hpp"

int main(int argc, char** argv){
    std::string input; 
    std::string outDir="data/tiles"; int tileW=32, tileH=32; std::string meta="data/tiles/meta.txt";
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="-i" && i+1<argc) input=argv[++i];
        else if(a=="-o" && i+1<argc) outDir=argv[++i];
        else if(a=="--tile" && i+1<argc){ std::string v=argv[++i]; auto pos=v.find('x'); if(pos!=std::string::npos){ tileW=std::stoi(v.substr(0,pos)); tileH=std::stoi(v.substr(pos+1)); }}
        else if(a=="--meta" && i+1<argc) meta=argv[++i];
        else if(a=="-h"){
            std::cout << "Usage: split_tool -i <input_map.png> [-o out_dir] [--tile WxH] [--meta meta_file]\n"; return 0;
        }
    }
    if(input.empty()){ std::cerr<<"Input PNG map required (-i).\n"; return 1; }
    try{
        TileSplitter splitter; auto tiles = splitter.split(input, outDir, tileW, tileH);
        TileIndex index; index.setTiles(tiles); if(!index.save(meta)){ std::cerr<<"Failed save meta\n"; return 2; }
        std::cout << "Split OK: "<<tiles.size()<<" tiles. Meta: "<<meta<<"\n";
    }catch(const std::exception& e){ std::cerr<<"Error: "<<e.what()<<"\n"; return 3; }
    return 0;
}
