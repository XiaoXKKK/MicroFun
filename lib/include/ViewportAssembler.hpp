#pragma once
#include <string>
#include <vector>
#include "TileIndex.hpp"

class ViewportAssembler {
public:
    bool assemble(const TileIndex& index, const Viewport& vp, const std::string& outFile) const;
};
