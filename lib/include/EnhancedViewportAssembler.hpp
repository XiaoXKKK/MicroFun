#pragma once
#include <memory>
#include <string>
#include <vector>
#include <future>

#include "TileIndex.hpp"
#include "TileCache.hpp"
#include "AsyncTileLoader.hpp"

class EnhancedViewportAssembler {
public:
    struct Config {
        bool enableAsyncLoading;
        bool enableCaching;
        int loadTimeoutMs;
        bool enablePreloading;
        bool fallbackToSync;
        
        Config() : enableAsyncLoading(true), enableCaching(true), 
                  loadTimeoutMs(5000), enablePreloading(true), fallbackToSync(true) {}
    };
    
    struct AssemblyStats {
        size_t totalTiles = 0;
        size_t cachedTiles = 0;
        size_t asyncLoadedTiles = 0;
        size_t syncLoadedTiles = 0;
        size_t failedTiles = 0;
        double assemblyTimeMs = 0.0;
        double avgLoadTimeMs = 0.0;
        
        double getCacheHitRate() const {
            if (totalTiles == 0) return 0.0;
            return static_cast<double>(cachedTiles) / totalTiles;
        }
    };
    
    explicit EnhancedViewportAssembler(std::shared_ptr<TileCache> cache = nullptr,
                                     std::shared_ptr<AsyncTileLoader> loader = nullptr,
                                     const Config& config = Config());
    
    bool assemble(const TileIndex& index, const Viewport& vp,
                  const std::string& resourceDir,
                  const std::string& outFile);
    
    std::string assembleToHex(const TileIndex& index, const Viewport& vp,
                              const std::string& resourceDir);
    
    std::future<bool> assembleAsync(const TileIndex& index, const Viewport& vp,
                                   const std::string& resourceDir,
                                   const std::string& outFile);
    
    void preloadNextViewport(const TileIndex& index, const Viewport& currentVp,
                            const Viewport& nextVp, const std::string& resourceDir);
    
    void preloadByMovement(const TileIndex& index, const Viewport& currentVp,
                          int deltaX, int deltaY, const std::string& resourceDir);
    
    void evictOutOfViewportTiles(const Viewport& vp, const TileIndex& index);
    
    AssemblyStats getLastAssemblyStats() const { return lastStats_; }
    
    void printCacheStatistics() const;
    
    void printLoaderStatistics() const;

private:
    std::shared_ptr<TileCache> cache_;
    std::shared_ptr<AsyncTileLoader> loader_;
    Config config_;
    
    mutable AssemblyStats lastStats_;
    
    struct TileRenderData {
        std::string tileId;
        std::vector<unsigned char> data;
        int width = 0;
        int height = 0;
        int channels = 0;
        bool isPureColor = false;
        uint32_t pureColorValue = 0;
        bool loaded = false;
    };
    
    void blit(std::vector<unsigned char>& canvas, int canvas_w, int canvas_h,
              const unsigned char* src, int sw, int sh, int stride, int dstX,
              int dstY) const;
    
    void blitSolidColor(std::vector<unsigned char>& canvas, int canvas_w,
                        int canvas_h, uint32_t color, int w, int h, int dstX,
                        int dstY) const;
    
    TileRenderData loadTileData(const TileMeta& tileMeta, const std::string& resourceDir);
    
    TileRenderData loadTileSync(const TileMeta& tileMeta, const std::string& resourceDir);
    
    std::vector<TileRenderData> loadTilesAsync(const std::vector<TileMeta>& tiles,
                                              const std::string& resourceDir);
    
    void renderTilesOnCanvas(std::vector<unsigned char>& canvas, 
                            const Viewport& vp,
                            const std::vector<TileMeta>& tiles,
                            const std::vector<TileRenderData>& tileData);
    
    static bool isPureColorTile(const std::string& fileName);
    
    static uint32_t parseColorFromFileName(const std::string& fileName);
    
    std::string generateTileId(const TileMeta& tileMeta) const;
};