#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "QuadTreeIndex.hpp"
#include "TileIndex.hpp"
#include "ViewportAssembler.hpp"
#include "stb_image.h"

int main(int argc, char** argv) {
    std::string resourceDir = "data/tiles";
    int px = 0, py = 0, sw = 128, sh = 128;
    bool outputPNG = false;
    std::string outFile = "";  // backward compat if -o specified
    bool useQuadTree = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-i" && i + 1 < argc)
            resourceDir = argv[++i];
        else if (a == "-p" && i + 1 < argc) {
            std::string v = argv[++i];
            auto c = v.find(',');
            if (c != std::string::npos) {
                px = std::stoi(v.substr(0, c));
                py = std::stoi(v.substr(c + 1));
            }
        } else if (a == "-s" && i + 1 < argc) {
            std::string v = argv[++i];
            auto x = v.find(',');
            if (x != std::string::npos) {
                sw = std::stoi(v.substr(0, x));
                sh = std::stoi(v.substr(x + 1));
            } else {  // fallback accept WxH
                x = v.find('x');
                if (x != std::string::npos) {
                    sw = std::stoi(v.substr(0, x));
                    sh = std::stoi(v.substr(x + 1));
                }
            }
        } else if (a == "-o" && i + 1 < argc) {
            outputPNG = true;
            outFile = argv[++i];
        } else if (a == "-q" || a == "--quadtree") {
            useQuadTree = true;
        } else if (a == "-h") {
            std::cout
                << "Usage: check_tool -i <resource_dir> -p posx,posy -s w,h "
                   "[-q|--quadtree] [-o <output.png>]\n";
            return 0;
        }
    }
    std::string meta = resourceDir + "/meta.txt";
    std::unique_ptr<TileIndex> index;
    if (useQuadTree) {
        index = std::make_unique<QuadTreeIndex>();
    } else {
        index = std::make_unique<TileIndex>();
    }
    if (!index->load(meta)) {
        std::cerr << "Failed load meta: " << meta << "\n";
        return 1;
    }
    // 输入 posy 以左下角为 0，需要转换为内部使用的“原图顶左为(0,0)”坐标。
    int internalY =
        index->getMapHeight() - py - sh;  // bottom-origin -> top-origin
    if (internalY < 0) internalY = 0;     // clamp
    ViewportAssembler assembler;
    Viewport vp{px, internalY, sw, sh};
    if (outputPNG) {
        std::string png =
            outFile.empty() ? (resourceDir + "/viewport.png") : outFile;
        if (!assembler.assemble(*index, vp, resourceDir, png)) {
            std::cerr << "Assemble failed\n";
            return 2;
        }
        std::cout << "Assemble OK -> " << png << "\n";
        return 0;
    }

    // hex dump pathway
    std::string hexResult = assembler.assembleToHex(*index, vp, resourceDir);
    if (hexResult.empty()) {
        return 1; // error message already printed by assembler
    }
    std::cout << hexResult << std::endl;

    return 0;
}
