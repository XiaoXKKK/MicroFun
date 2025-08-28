#pragma once
#include <string>
#include <vector>

#include "TileIndex.hpp"

class ViewportAssembler {
   public:
    bool assemble(const TileIndex& index, const Viewport& vp,
                  const std::string& resourceDir,
                  const std::string& outFile) const;
    std::string assembleToHex(const TileIndex& index, const Viewport& vp,
                              const std::string& resourceDir) const;

   private:
    void blit(std::vector<unsigned char>& canvas, int canvas_w, int canvas_h,
              const unsigned char* src, int sw, int sh, int stride, int dstX,
              int dstY) const;
};
