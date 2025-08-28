#include "TileCache.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>

TileCache::TileCache(const Config& config) : config_(config) {
    stats_ = Statistics{};
}

std::shared_ptr<CachedTile> TileCache::get(const std::string& tileId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(tileId);
    if (it != cache_.end()) {
        stats_.cacheHits++;
        
        it->second->updateAccessTime();
        
        if (config_.enableLRU) {
            moveToFront(tileId);
        }
        
        return it->second;
    }
    
    stats_.cacheMisses++;
    return nullptr;
}

void TileCache::put(const std::string& tileId, std::vector<unsigned char>&& data, 
                   int width, int height, int channels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto existingIt = cache_.find(tileId);
    if (existingIt != cache_.end()) {
        removeTile(tileId);
    }
    
    size_t tileSize = data.size() + sizeof(CachedTile) + tileId.size();
    
    evictIfNeeded();
    
    while ((stats_.totalMemoryUsed + tileSize > config_.maxMemoryBytes || 
            stats_.totalTiles >= config_.maxTileCount) && !cache_.empty()) {
        evictLRU();
    }
    
    auto tile = std::make_shared<CachedTile>(tileId, std::move(data), width, height, channels);
    
    cache_[tileId] = tile;
    stats_.totalMemoryUsed += tileSize;
    stats_.totalTiles++;
    
    if (config_.enableLRU) {
        lruList_.push_front(tileId);
        lruMap_[tileId] = lruList_.begin();
    }
}

void TileCache::putPureColor(const std::string& tileId, uint32_t color, int width, int height) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto existingIt = cache_.find(tileId);
    if (existingIt != cache_.end()) {
        removeTile(tileId);
    }
    
    std::vector<unsigned char> emptyData;
    size_t tileSize = sizeof(CachedTile) + tileId.size();
    
    evictIfNeeded();
    
    while ((stats_.totalMemoryUsed + tileSize > config_.maxMemoryBytes || 
            stats_.totalTiles >= config_.maxTileCount) && !cache_.empty()) {
        evictLRU();
    }
    
    auto tile = std::make_shared<CachedTile>(tileId, std::move(emptyData), 
                                            width, height, 4, true, color);
    
    cache_[tileId] = tile;
    stats_.totalMemoryUsed += tileSize;
    stats_.totalTiles++;
    
    if (config_.enableLRU) {
        lruList_.push_front(tileId);
        lruMap_[tileId] = lruList_.begin();
    }
}

void TileCache::evictOutOfViewport(const std::vector<std::string>& visibleTileIds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::unordered_set<std::string> visibleSet(visibleTileIds.begin(), visibleTileIds.end());
    
    std::vector<std::string> toEvict;
    for (const auto& pair : cache_) {
        if (visibleSet.find(pair.first) == visibleSet.end()) {
            toEvict.push_back(pair.first);
        }
    }
    
    for (const std::string& tileId : toEvict) {
        removeTile(tileId);
        stats_.evictedTiles++;
    }
}

void TileCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    cache_.clear();
    lruList_.clear();
    lruMap_.clear();
    
    stats_.totalMemoryUsed = 0;
    stats_.totalTiles = 0;
}

TileCache::Statistics TileCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

size_t TileCache::getMemoryUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_.totalMemoryUsed;
}

size_t TileCache::getTileCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_.totalTiles;
}

void TileCache::moveToFront(const std::string& tileId) {
    auto mapIt = lruMap_.find(tileId);
    if (mapIt != lruMap_.end()) {
        lruList_.erase(mapIt->second);
        lruList_.push_front(tileId);
        lruMap_[tileId] = lruList_.begin();
    }
}

void TileCache::evictLRU() {
    if (lruList_.empty()) {
        if (!cache_.empty()) {
            auto it = cache_.begin();
            removeTile(it->first);
        }
        return;
    }
    
    std::string oldestTileId = lruList_.back();
    removeTile(oldestTileId);
    stats_.evictedTiles++;
}

void TileCache::evictIfNeeded() {
    while ((stats_.totalMemoryUsed > config_.maxMemoryBytes || 
            stats_.totalTiles >= config_.maxTileCount) && !cache_.empty()) {
        evictLRU();
    }
}

void TileCache::removeTile(const std::string& tileId) {
    auto cacheIt = cache_.find(tileId);
    if (cacheIt != cache_.end()) {
        stats_.totalMemoryUsed -= cacheIt->second->sizeBytes + sizeof(CachedTile) + tileId.size();
        stats_.totalTiles--;
        
        cache_.erase(cacheIt);
        
        auto lruMapIt = lruMap_.find(tileId);
        if (lruMapIt != lruMap_.end()) {
            lruList_.erase(lruMapIt->second);
            lruMap_.erase(lruMapIt);
        }
    }
}