#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

struct TileMeta {
    int x;
    int y;
    int w;
    int h;
    std::string file;
};

class TileSplitter {
public:
    // Split a PNG map image into RGBA tiles (written as individual PNG files).
    // Returns metadata for produced tiles.
    std::vector<TileMeta> split(const std::string& inputPath, const std::string& outDir,
                                int tileW, int tileH);
};
