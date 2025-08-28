#include "ViewportAssembler.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "stb_image.h"
#include "stb_image_write.h"

using namespace std;

void ViewportAssembler::blit(std::vector<unsigned char>& canvas, int canvas_w,
                             int canvas_h, const unsigned char* src, int sw,
                             int sh, int stride, int dstX, int dstY) const {
    for (int y = 0; y < sh; ++y) {
        if (dstY + y < 0 || dstY + y >= canvas_h) continue;
        unsigned char* dstRow = &canvas[(dstY + y) * canvas_w * 4];
        const unsigned char* srcRow = src + y * stride;
        for (int x = 0; x < sw; ++x) {
            if (dstX + x < 0 || dstX + x >= canvas_w) continue;
            const unsigned char* sp = &srcRow[x * 4];
            unsigned char* dp = &dstRow[(dstX + x) * 4];
            // alpha blend simple over
            float a = sp[3] / 255.0f;
            for (int c = 0; c < 3; ++c) {
                dp[c] =
                    static_cast<unsigned char>(sp[c] * a + dp[c] * (1 - a));
            }
            dp[3] = static_cast<unsigned char>(
                min(255.0f, sp[3] + dp[3] * (1 - a)));
        }
    }
}

bool ViewportAssembler::assemble(const TileIndex& index, const Viewport& vp,
                                 const string& resourceDir,
                                 const string& outFile) const {
    using clock = std::chrono::high_resolution_clock;
    auto t0 = clock::now();
    auto tiles = index.query(vp);
    if (tiles.empty()) {
        cerr << "No tiles overlap viewport\n";
        return false;
    }
    // RGBA buffer for viewport
    vector<unsigned char> canvas(vp.w * vp.h * 4, 0);
    // load each tile (assume current working dir contains tile files or provide
    // relative path externally)
    for (auto& t : tiles) {
        int w, h, c;
        unsigned char* data =
            stbi_load((resourceDir + "/" + t.file).c_str(), &w, &h, &c, 4);
        if (!data) {
            cerr << "Failed load tile " << t.file << "\n";
            continue;
        }
        int localX = t.x - vp.x;
        int localY = t.y - vp.y;
        blit(canvas, vp.w, vp.h, data, w, h, w * 4, localX, localY);
        stbi_image_free(data);
    }

    // write viewport png
    if (!stbi_write_png(outFile.c_str(), vp.w, vp.h, 4, canvas.data(),
                        vp.w * 4)) {
        cerr << "Failed write viewport png\n";
        return false;
    }
    auto t1 = clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    cerr << "Assemble time: " << ms << " ms (viewport " << vp.w << "x" << vp.h
         << ", tiles=";
    cerr << tiles.size() << ")\n";
    return true;
}

std::string ViewportAssembler::assembleToHex(
    const TileIndex& index, const Viewport& vp,
    const std::string& resourceDir) const {
    auto tiles = index.query(vp);
    if (tiles.empty()) {
        cerr << "No tiles overlap viewport\n";
        return "";
    }
    std::vector<unsigned char> canvas(vp.w * vp.h * 4, 0);
    for (auto& t : tiles) {
        int w, h, c;
        std::string tilePath = resourceDir + "/" + t.file;
        unsigned char* data = stbi_load(tilePath.c_str(), &w, &h, &c, 4);
        if (!data) {
            std::cerr << "Fail tile " << t.file << "\n";
            continue;
        }
        int lx = t.x - vp.x;
        int ly = t.y - vp.y;
        blit(canvas, vp.w, vp.h, data, w, h, w * 4, lx, ly);
        stbi_image_free(data);
    }
    // output hex values
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    size_t count = vp.w * vp.h;
    for (size_t i = 0; i < count; ++i) {
        unsigned char r = canvas[i * 4 + 0];
        unsigned char g = canvas[i * 4 + 1];
        unsigned char b = canvas[i * 4 + 2];
        unsigned char a = canvas[i * 4 + 3];
        uint32_t v = (r << 24) | (g << 16) | (b << 8) | a;  // 0xRRGGBBAA
        ss << "0x" << std::setw(8) << v;
        if (i + 1 < count) ss << ",";
    }
    return ss.str();
}