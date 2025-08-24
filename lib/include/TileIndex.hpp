#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "TileSplitter.hpp"

struct Viewport {
    int x; // top-left world coord
    int y;
    int w;
    int h;
};

class TileIndex {
public:
    bool load(const std::string& metaFile);
    std::vector<TileMeta> query(const Viewport& vp) const;
    bool save(const std::string& metaFile) const; // for split phase
    void setTiles(std::vector<TileMeta> tiles);
private:
    std::vector<TileMeta> tiles_;
};
