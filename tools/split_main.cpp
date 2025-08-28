#include <iostream>
#include <string>
#include <filesystem>
#include "TileSplitter.hpp"
#include "QuadTreeSplitter.hpp"
#include "TileIndex.hpp"

int main(int argc, char** argv){
    std::string input; 
    std::string outDir="data/tiles"; // default output dir
    int tileW=32, tileH=32; // default tile size (kept for backward compatibility)
    std::string meta; // will derive: <outDir>/meta.txt
    
    // 四叉树分割参数
    bool useQuadTree = false;
    QuadTreeSplitter::Config quadTreeConfig;
    bool compareMode = false; // 对比模式
    
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="-i" && i+1<argc) input=argv[++i];
        else if(a=="-o" && i+1<argc) outDir=argv[++i];
        else if(a=="--tile" && i+1<argc){ // hidden optional
            std::string v=argv[++i]; auto pos=v.find('x'); if(pos!=std::string::npos){ tileW=std::stoi(v.substr(0,pos)); tileH=std::stoi(v.substr(pos+1)); }
        }
        else if(a=="--meta" && i+1<argc){ meta=argv[++i]; }
        else if(a=="--quadtree"){
            useQuadTree = true;
        }
        else if(a=="--max-depth" && i+1<argc){
            quadTreeConfig.maxDepth = std::stoi(argv[++i]);
        }
        else if(a=="--min-size" && i+1<argc){
            quadTreeConfig.minTileSize = std::stoi(argv[++i]);
        }
        else if(a=="--color-tolerance" && i+1<argc){
            quadTreeConfig.colorTolerance = std::stoi(argv[++i]);
        }
        else if(a=="--compare"){
            compareMode = true;
        }
        else if(a=="-h"){
            std::cout << "Usage: split_tool -i <input_map.png> -o <output_dir> [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --quadtree              Use quad-tree splitting based on color uniformity\n";
            std::cout << "  --max-depth <depth>     Maximum quad-tree depth (default: 8)\n";
            std::cout << "  --min-size <size>       Minimum tile size (default: 4)\n";
            std::cout << "  --color-tolerance <tol> Color comparison tolerance (default: 0)\n";
            std::cout << "  --compare               Generate both fixed-size and quad-tree results\n";
            std::cout << "  --tile <WxH>            Fixed tile size (default: 32x32)\n";
            std::cout << "  --meta <file>           Meta file path (default: <output_dir>/meta.txt)\n";
            return 0;
        }
    }
    if(input.empty()){ 
        std::cerr<<"Input PNG map required (-i).\n"; 
        return 1; 
    }
    if(meta.empty()) meta = outDir + "/meta.txt";
    
    try{
        std::vector<TileMeta> tiles;
        
        if(compareMode) {
            // 对比模式：生成两种分割结果
            std::cout << "Compare mode: generating both fixed-size and quad-tree results...\n";
            
            // 固定尺寸分割
            std::string fixedOutDir = outDir + "_fixed";
            std::string fixedMeta = fixedOutDir + "/meta.txt";
            TileSplitter fixedSplitter;
            auto fixedTiles = fixedSplitter.split(input, fixedOutDir, tileW, tileH);
            TileIndex fixedIndex;
            fixedIndex.setTiles(fixedTiles);
            if(!fixedIndex.save(fixedMeta)){
                std::cerr<<"Failed to save fixed meta\n"; 
                return 2; 
            }
            std::cout << "Fixed-size split: " << fixedTiles.size() << " tiles. Meta: " << fixedMeta << "\n";
            
            // 四叉树分割
            std::string quadOutDir = outDir + "_quadtree";
            std::string quadMeta = quadOutDir + "/meta.txt";
            QuadTreeSplitter quadSplitter;
            auto quadTiles = quadSplitter.splitQuadTree(input, quadOutDir, quadTreeConfig);
            TileIndex quadIndex;
            quadIndex.setTiles(quadTiles);
            if(!quadIndex.save(quadMeta)){
                std::cerr<<"Failed to save quad-tree meta\n"; 
                return 2; 
            }
            std::cout << "Quad-tree split: " << quadTiles.size() << " tiles. Meta: " << quadMeta << "\n";
            
            // 显示对比结果
            std::cout << "\nComparison Results:\n";
            std::cout << "Fixed-size tiles: " << fixedTiles.size() << "\n";
            std::cout << "Quad-tree tiles: " << quadTiles.size() << "\n";
            std::cout << "Reduction ratio: " << (float)quadTiles.size() / fixedTiles.size() * 100.0f << "%\n";
            
        } else if(useQuadTree) {
            // 四叉树分割模式
            std::cout << "Using quad-tree splitting with config:\n";
            std::cout << "  Max depth: " << quadTreeConfig.maxDepth << "\n";
            std::cout << "  Min tile size: " << quadTreeConfig.minTileSize << "\n";
            std::cout << "  Color tolerance: " << quadTreeConfig.colorTolerance << "\n";
            
            QuadTreeSplitter splitter;
            tiles = splitter.splitQuadTree(input, outDir, quadTreeConfig);
        } else {
            // 传统固定尺寸分割模式
            std::cout << "Using fixed-size splitting: " << tileW << "x" << tileH << "\n";
            TileSplitter splitter;
            tiles = splitter.split(input, outDir, tileW, tileH);
        }
        
        if(!compareMode) {
            TileIndex index;
            index.setTiles(tiles);
            if(!index.save(meta)){
                std::cerr<<"Failed to save meta\n"; 
                return 2; 
            }
            std::cout << "Split completed: " << tiles.size() << " tiles. Meta: " << meta << "\n";
        }
        
    }catch(const std::exception& e){ 
        std::cerr<<"Error: "<<e.what()<<"\n"; 
        return 3; 
    }
    return 0;
}
