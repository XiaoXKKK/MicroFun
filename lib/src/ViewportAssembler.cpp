#include "ViewportAssembler.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

bool ViewportAssembler::assemble(const TileIndex &index, const Viewport &vp,
                                 const string &outFile) const {
  auto tiles = index.query(vp);
  if (tiles.empty()) {
    cerr << "No tiles overlap viewport\n";
    return false;
  }
  // RGBA buffer for viewport
  vector<unsigned char> canvas(vp.w * vp.h * 4, 0);
  auto blit = [&](const unsigned char *src, int sw, int sh, int stride,
                  int dstX, int dstY) {
    for (int y = 0; y < sh; ++y) {
      if (dstY + y < 0 || dstY + y >= vp.h)
        continue;
      unsigned char *dstRow = &canvas[(dstY + y) * vp.w * 4];
      const unsigned char *srcRow = src + y * stride;
      for (int x = 0; x < sw; ++x) {
        if (dstX + x < 0 || dstX + x >= vp.w)
          continue;
        const unsigned char *sp = &srcRow[x * 4];
        unsigned char *dp = &dstRow[(dstX + x) * 4];
        // alpha blend simple over
        float a = sp[3] / 255.0f;
        for (int c = 0; c < 3; ++c) {
          dp[c] = static_cast<unsigned char>(sp[c] * a + dp[c] * (1 - a));
        }
        dp[3] =
            static_cast<unsigned char>(min(255.0f, sp[3] + dp[3] * (1 - a)));
      }
    }
  };
  // load each tile (assume current working dir contains tile files or provide
  // relative path externally)
  for (auto &t : tiles) {
    int w, h, c;
    unsigned char *data = stbi_load((string("data/tiles/") + t.file).c_str(), &w, &h, &c, 4);
    if (!data) {
      cerr << "Failed load tile " << t.file << "\n";
      continue;
    }
    int localX = t.x - vp.x;
    int localY = t.y - vp.y;
    blit(data, w, h, w * 4, localX, localY);
    stbi_image_free(data);
  }
  // write viewport png
  if (!stbi_write_png(outFile.c_str(), vp.w, vp.h, 4, canvas.data(),
                      vp.w * 4)) {
    cerr << "Failed write viewport png\n";
    return false;
  }
  return true;
}
