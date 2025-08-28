#pragma once
#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CachedTile {
    std::string tileId;
    std::vector<unsigned char> data;
    int width;
    int height;
    int channels;
    size_t sizeBytes;
    std::chrono::steady_clock::time_point lastAccessed;
    bool isPureColor;
    uint32_t pureColorValue;
    
    CachedTile(const std::string& id, std::vector<unsigned char>&& tileData, 
               int w, int h, int c, bool isPure = false, uint32_t color = 0)
        : tileId(id), data(std::move(tileData)), width(w), height(h), 
          channels(c), sizeBytes(data.size()), isPureColor(isPure), 
          pureColorValue(color) {
        lastAccessed = std::chrono::steady_clock::now();
    }
    
    void updateAccessTime() {
        lastAccessed = std::chrono::steady_clock::now();
    }
};

class TileCache {
public:
    struct Config {
        size_t maxMemoryBytes;
        size_t maxTileCount;
        bool enableLRU;
        
        Config() : maxMemoryBytes(512 * 1024 * 1024), maxTileCount(10000), enableLRU(true) {}
    };
    
    struct Statistics {
        size_t totalMemoryUsed = 0;
        size_t totalTiles = 0;
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
        size_t evictedTiles = 0;
        
        double getHitRate() const {
            if (cacheHits + cacheMisses == 0) return 0.0;
            return static_cast<double>(cacheHits) / (cacheHits + cacheMisses);
        }
    };
    
    explicit TileCache(const Config& config = Config());
    ~TileCache() = default;
    
    TileCache(const TileCache&) = delete;
    TileCache& operator=(const TileCache&) = delete;
    
    std::shared_ptr<CachedTile> get(const std::string& tileId);
    
    void put(const std::string& tileId, std::vector<unsigned char>&& data, 
             int width, int height, int channels);
    
    void putPureColor(const std::string& tileId, uint32_t color, int width, int height);
    
    void evictOutOfViewport(const std::vector<std::string>& visibleTileIds);
    
    void clear();
    
    Statistics getStatistics() const;
    
    size_t getMemoryUsage() const;
    
    size_t getTileCount() const;

private:
    mutable std::mutex mutex_;
    Config config_;
    
    std::unordered_map<std::string, std::shared_ptr<CachedTile>> cache_;
    
    std::list<std::string> lruList_;
    std::unordered_map<std::string, std::list<std::string>::iterator> lruMap_;
    
    mutable Statistics stats_;
    
    void moveToFront(const std::string& tileId);
    
    void evictLRU();
    
    void evictIfNeeded();
    
    void removeTile(const std::string& tileId);
    
    size_t estimateTileSize(int width, int height, int channels, const std::string& tileId) const {
        return width * height * channels + sizeof(CachedTile) + tileId.size();
    }
};