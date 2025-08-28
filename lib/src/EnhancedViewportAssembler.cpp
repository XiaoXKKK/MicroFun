#include "EnhancedViewportAssembler.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "stb_image.h"
#include "stb_image_write.h"

EnhancedViewportAssembler::EnhancedViewportAssembler(std::shared_ptr<TileCache> cache,
                                                   std::shared_ptr<AsyncTileLoader> loader,
                                                   const Config& config)
    : cache_(cache), loader_(loader), config_(config) {
    
    if (!cache_ && config_.enableCaching) {
        cache_ = std::make_shared<TileCache>();
    }
    
    if (!loader_ && config_.enableAsyncLoading) {
        if (cache_) {
            loader_ = std::make_shared<AsyncTileLoader>(cache_);
            loader_->start();
        }
    }
}

bool EnhancedViewportAssembler::assemble(const TileIndex& index, const Viewport& vp,
                                        const std::string& resourceDir,
                                        const std::string& outFile) {
    using clock = std::chrono::high_resolution_clock;
    auto t0 = clock::now();
    
    lastStats_ = AssemblyStats{};
    
    auto tiles = index.query(vp);
    if (tiles.empty()) {
        std::cerr << "No tiles overlap viewport\n";
        return false;
    }
    
    lastStats_.totalTiles = tiles.size();
    
    std::vector<unsigned char> canvas(vp.w * vp.h * 4, 0);
    
    std::vector<TileRenderData> tileData;
    
    if (config_.enableAsyncLoading && loader_) {
        tileData = loadTilesAsync(tiles, resourceDir);
    } else {
        tileData.reserve(tiles.size());
        for (const auto& tileMeta : tiles) {
            tileData.push_back(loadTileSync(tileMeta, resourceDir));
        }
    }
    
    renderTilesOnCanvas(canvas, vp, tiles, tileData);
    
    if (!stbi_write_png(outFile.c_str(), vp.w, vp.h, 4, canvas.data(), vp.w * 4)) {
        std::cerr << "Failed write viewport png\n";
        return false;
    }
    
    auto t1 = clock::now();
    lastStats_.assemblyTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    
    if (config_.enablePreloading && loader_) {
        Viewport expandedVp{vp.x - vp.w/4, vp.y - vp.h/4, vp.w + vp.w/2, vp.h + vp.h/2};
        auto preloadTiles = index.query(expandedVp);
        loader_->preloadViewportTiles(preloadTiles, resourceDir, 50);
    }
    
    std::cerr << "Enhanced assemble time: " << lastStats_.assemblyTimeMs << " ms (viewport " 
              << vp.w << "x" << vp.h << ", tiles=" << tiles.size() 
              << ", cache_hits=" << lastStats_.cachedTiles << ")\n";
    
    return true;
}

std::string EnhancedViewportAssembler::assembleToHex(const TileIndex& index, const Viewport& vp,
                                                    const std::string& resourceDir) {
    lastStats_ = AssemblyStats{};
    
    auto tiles = index.query(vp);
    if (tiles.empty()) {
        std::cerr << "No tiles overlap viewport\n";
        return "";
    }
    
    lastStats_.totalTiles = tiles.size();
    
    std::vector<unsigned char> canvas(vp.w * vp.h * 4, 0);
    
    std::vector<TileRenderData> tileData;
    
    if (config_.enableAsyncLoading && loader_) {
        tileData = loadTilesAsync(tiles, resourceDir);
    } else {
        tileData.reserve(tiles.size());
        for (const auto& tileMeta : tiles) {
            tileData.push_back(loadTileSync(tileMeta, resourceDir));
        }
    }
    
    renderTilesOnCanvas(canvas, vp, tiles, tileData);
    
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    size_t count = vp.w * vp.h;
    for (size_t i = 0; i < count; ++i) {
        unsigned char r = canvas[i * 4 + 0];
        unsigned char g = canvas[i * 4 + 1];
        unsigned char b = canvas[i * 4 + 2];
        unsigned char a = canvas[i * 4 + 3];
        uint32_t v = (r << 24) | (g << 16) | (b << 8) | a;
        ss << "0x" << std::setw(8) << v;
        if (i + 1 < count) ss << ",";
    }
    
    return ss.str();
}

std::future<bool> EnhancedViewportAssembler::assembleAsync(const TileIndex& index, const Viewport& vp,
                                                          const std::string& resourceDir,
                                                          const std::string& outFile) {
    return std::async(std::launch::async, [this, &index, vp, resourceDir, outFile]() {
        return assemble(index, vp, resourceDir, outFile);
    });
}

void EnhancedViewportAssembler::preloadNextViewport(const TileIndex& index, const Viewport& currentVp,
                                                   const Viewport& nextVp, const std::string& resourceDir) {
    if (!config_.enablePreloading || !loader_) {
        return;
    }
    
    auto nextTiles = index.query(nextVp);
    loader_->preloadViewportTiles(nextTiles, resourceDir, 75);
}

void EnhancedViewportAssembler::preloadByMovement(const TileIndex& index, const Viewport& currentVp,
                                                 int deltaX, int deltaY, const std::string& resourceDir) {
    if (!config_.enablePreloading || !loader_) {
        return;
    }
    
    Viewport movement{deltaX, deltaY, 0, 0};
    loader_->preloadByDirection(currentVp, movement, index, resourceDir);
}

void EnhancedViewportAssembler::evictOutOfViewportTiles(const Viewport& vp, const TileIndex& index) {
    if (!cache_) {
        return;
    }
    
    auto visibleTiles = index.query(vp);
    std::vector<std::string> visibleTileIds;
    visibleTileIds.reserve(visibleTiles.size());
    
    for (const auto& tile : visibleTiles) {
        visibleTileIds.push_back(generateTileId(tile));
    }
    
    cache_->evictOutOfViewport(visibleTileIds);
}

void EnhancedViewportAssembler::printCacheStatistics() const {
    if (!cache_) {
        std::cout << "Cache not enabled\n";
        return;
    }
    
    auto stats = cache_->getStatistics();
    std::cout << "=== Cache Statistics ===\n";
    std::cout << "Memory usage: " << (stats.totalMemoryUsed / 1024 / 1024) << " MB\n";
    std::cout << "Total tiles: " << stats.totalTiles << "\n";
    std::cout << "Cache hits: " << stats.cacheHits << "\n";
    std::cout << "Cache misses: " << stats.cacheMisses << "\n";
    std::cout << "Hit rate: " << (stats.getHitRate() * 100) << "%\n";
    std::cout << "Evicted tiles: " << stats.evictedTiles << "\n";
}

void EnhancedViewportAssembler::printLoaderStatistics() const {
    if (!loader_) {
        std::cout << "Async loader not enabled\n";
        return;
    }
    
    auto stats = loader_->getStatistics();
    std::cout << "=== Async Loader Statistics ===\n";
    std::cout << "Total requests: " << stats.totalRequests << "\n";
    std::cout << "Completed loads: " << stats.completedLoads << "\n";
    std::cout << "Failed loads: " << stats.failedLoads << "\n";
    std::cout << "Cache hits: " << stats.cacheHits << "\n";
    std::cout << "Queued requests: " << stats.queuedRequests << "\n";
    std::cout << "Active loads: " << stats.activeLoads << "\n";
    std::cout << "Success rate: " << (stats.getSuccessRate() * 100) << "%\n";
}

void EnhancedViewportAssembler::blit(std::vector<unsigned char>& canvas, int canvas_w,
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
            
            float a = sp[3] / 255.0f;
            for (int c = 0; c < 3; ++c) {
                dp[c] = static_cast<unsigned char>(sp[c] * a + dp[c] * (1 - a));
            }
            dp[3] = static_cast<unsigned char>(
                std::min(255.0f, sp[3] + dp[3] * (1 - a)));
        }
    }
}

void EnhancedViewportAssembler::blitSolidColor(std::vector<unsigned char>& canvas,
                                              int canvas_w, int canvas_h,
                                              uint32_t color, int w, int h, int dstX,
                                              int dstY) const {
    unsigned char r = (color >> 24) & 0xFF;
    unsigned char g = (color >> 16) & 0xFF;
    unsigned char b = (color >> 8) & 0xFF;
    unsigned char a = color & 0xFF;
    
    float alpha = a / 255.0f;
    
    for (int y = 0; y < h; ++y) {
        if (dstY + y < 0 || dstY + y >= canvas_h) continue;
        unsigned char* dstRow = &canvas[(dstY + y) * canvas_w * 4];
        
        for (int x = 0; x < w; ++x) {
            if (dstX + x < 0 || dstX + x >= canvas_w) continue;
            unsigned char* dp = &dstRow[(dstX + x) * 4];
            
            dp[0] = static_cast<unsigned char>(r * alpha + dp[0] * (1 - alpha));
            dp[1] = static_cast<unsigned char>(g * alpha + dp[1] * (1 - alpha));
            dp[2] = static_cast<unsigned char>(b * alpha + dp[2] * (1 - alpha));
            dp[3] = static_cast<unsigned char>(
                std::min(255.0f, a + dp[3] * (1 - alpha)));
        }
    }
}

EnhancedViewportAssembler::TileRenderData EnhancedViewportAssembler::loadTileData(
    const TileMeta& tileMeta, const std::string& resourceDir) {
    
    TileRenderData result;
    result.tileId = generateTileId(tileMeta);
    
    if (cache_) {
        auto cachedTile = cache_->get(result.tileId);
        if (cachedTile) {
            lastStats_.cachedTiles++;
            result.loaded = true;
            result.width = cachedTile->width;
            result.height = cachedTile->height;
            result.channels = cachedTile->channels;
            result.isPureColor = cachedTile->isPureColor;
            result.pureColorValue = cachedTile->pureColorValue;
            
            if (!cachedTile->isPureColor) {
                result.data = cachedTile->data;
            }
            
            return result;
        }
    }
    
    return loadTileSync(tileMeta, resourceDir);
}

EnhancedViewportAssembler::TileRenderData EnhancedViewportAssembler::loadTileSync(
    const TileMeta& tileMeta, const std::string& resourceDir) {
    
    TileRenderData result;
    result.tileId = generateTileId(tileMeta);
    
    if (isPureColorTile(tileMeta.file)) {
        result.isPureColor = true;
        result.pureColorValue = parseColorFromFileName(tileMeta.file);
        result.width = tileMeta.w;
        result.height = tileMeta.h;
        result.channels = 4;
        result.loaded = true;
        
        if (cache_) {
            cache_->putPureColor(result.tileId, result.pureColorValue, 
                               result.width, result.height);
        }
        
        lastStats_.syncLoadedTiles++;
    } else {
        std::string filePath = resourceDir + "/" + tileMeta.file;
        int w, h, c;
        unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &c, 4);
        
        if (data) {
            result.width = w;
            result.height = h;
            result.channels = 4;
            result.loaded = true;
            
            size_t dataSize = w * h * 4;
            result.data.resize(dataSize);
            std::memcpy(result.data.data(), data, dataSize);
            
            stbi_image_free(data);
            
            if (cache_) {
                std::vector<unsigned char> dataCopy = result.data;
                cache_->put(result.tileId, std::move(dataCopy), w, h, 4);
            }
            
            lastStats_.syncLoadedTiles++;
        } else {
            std::cerr << "Failed to load tile: " << tileMeta.file << "\n";
            lastStats_.failedTiles++;
        }
    }
    
    return result;
}

std::vector<EnhancedViewportAssembler::TileRenderData> 
EnhancedViewportAssembler::loadTilesAsync(const std::vector<TileMeta>& tiles,
                                         const std::string& resourceDir) {
    
    std::vector<TileRenderData> results;
    results.reserve(tiles.size());
    
    std::vector<std::future<LoadResult>> futures;
    futures.reserve(tiles.size());
    
    for (const auto& tileMeta : tiles) {
        std::string tileId = generateTileId(tileMeta);
        
        auto cachedTile = cache_ ? cache_->get(tileId) : nullptr;
        if (cachedTile) {
            TileRenderData cached;
            cached.tileId = tileId;
            cached.loaded = true;
            cached.width = cachedTile->width;
            cached.height = cachedTile->height;
            cached.channels = cachedTile->channels;
            cached.isPureColor = cachedTile->isPureColor;
            cached.pureColorValue = cachedTile->pureColorValue;
            
            if (!cachedTile->isPureColor) {
                cached.data = cachedTile->data;
            }
            
            results.push_back(std::move(cached));
            lastStats_.cachedTiles++;
        } else {
            auto future = loader_->loadTileAsync(tileId, resourceDir, tileMeta, 200);
            futures.push_back(std::move(future));
        }
    }
    
    for (auto& future : futures) {
        try {
            auto loadResult = future.get();
            
            TileRenderData tileData;
            tileData.tileId = loadResult.tileId;
            tileData.loaded = (loadResult.status == LoadStatus::Completed);
            
            if (tileData.loaded) {
                tileData.width = loadResult.width;
                tileData.height = loadResult.height;
                tileData.channels = loadResult.channels;
                tileData.isPureColor = loadResult.isPureColor;
                tileData.pureColorValue = loadResult.pureColorValue;
                
                if (!loadResult.isPureColor) {
                    tileData.data = std::move(loadResult.data);
                }
                
                lastStats_.asyncLoadedTiles++;
            } else {
                lastStats_.failedTiles++;
            }
            
            results.push_back(std::move(tileData));
            
        } catch (const std::exception& e) {
            std::cerr << "Async load failed: " << e.what() << "\n";
            
            TileRenderData failedTile;
            failedTile.loaded = false;
            results.push_back(std::move(failedTile));
            lastStats_.failedTiles++;
        }
    }
    
    return results;
}

void EnhancedViewportAssembler::renderTilesOnCanvas(std::vector<unsigned char>& canvas,
                                                   const Viewport& vp,
                                                   const std::vector<TileMeta>& tiles,
                                                   const std::vector<TileRenderData>& tileData) {
    
    for (size_t i = 0; i < tiles.size() && i < tileData.size(); ++i) {
        const auto& tileMeta = tiles[i];
        const auto& data = tileData[i];
        
        if (!data.loaded) {
            continue;
        }
        
        int localX = tileMeta.x - vp.x;
        int localY = tileMeta.y - vp.y;
        
        if (data.isPureColor) {
            blitSolidColor(canvas, vp.w, vp.h, data.pureColorValue, 
                          data.width, data.height, localX, localY);
        } else {
            blit(canvas, vp.w, vp.h, data.data.data(), data.width, data.height,
                data.width * 4, localX, localY);
        }
    }
}

bool EnhancedViewportAssembler::isPureColorTile(const std::string& fileName) {
    return fileName.length() == 8;
}

uint32_t EnhancedViewportAssembler::parseColorFromFileName(const std::string& fileName) {
    if (!isPureColorTile(fileName)) {
        return 0;
    }
    
    uint32_t color = 0;
    for (char c : fileName) {
        color <<= 4;
        if (c >= '0' && c <= '9') {
            color |= (c - '0');
        } else if (c >= 'A' && c <= 'F') {
            color |= (c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            color |= (c - 'a' + 10);
        }
    }
    
    return color;
}

std::string EnhancedViewportAssembler::generateTileId(const TileMeta& tileMeta) const {
    return tileMeta.file;
}