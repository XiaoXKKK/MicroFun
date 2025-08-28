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
    int getMapWidth() const { return mapWidth_; }
    int getMapHeight() const { return mapHeight_; }
protected:
    std::vector<TileMeta> tiles_;
    int mapWidth_ = 0;  // derived from tiles: max(x+w)
    int mapHeight_ = 0; // derived from tiles: max(y+h) (y 自顶向下递增)
};
