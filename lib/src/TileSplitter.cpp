#include "TileSplitter.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

vector<TileMeta> TileSplitter::split(const string& inputPath, const string& outDir, int tileW, int tileH) {
    if (!std::filesystem::exists(outDir)) {
        std::filesystem::create_directories(outDir);
    }
    int W,H,C;
    unsigned char* data = stbi_load(inputPath.c_str(), &W,&H,&C, 4); // force RGBA
    if(!data){
        throw runtime_error(string("Failed to load PNG: ") + inputPath + " reason=" + (stbi_failure_reason()?stbi_failure_reason():""));
    }
    vector<TileMeta> metas;
    int stride = W * 4;
    for(int y=0; y<H; y += tileH){
        for(int x=0; x<W; x += tileW){
            int cw = std::min(tileW, W - x);
            int ch = std::min(tileH, H - y);
            string tileName = "tile_" + to_string(x) + "_" + to_string(y) + ".png";
            string outPath = outDir + "/" + tileName;
            // Copy region into temporary buffer
            vector<unsigned char> buf(cw * ch * 4);
            for(int row=0; row<ch; ++row){
                memcpy(&buf[row*cw*4], data + (y+row)*stride + x*4, cw*4);
            }
            if(!stbi_write_png(outPath.c_str(), cw, ch, 4, buf.data(), cw*4)){
                cerr << "Warn: write tile failed " << outPath << "\n";
            }
            metas.push_back(TileMeta{ x, y, cw, ch, tileName });
        }
    }
    stbi_image_free(data);
    return metas;
}
