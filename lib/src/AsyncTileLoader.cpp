#include "AsyncTileLoader.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "stb_image.h"

AsyncTileLoader::AsyncTileLoader(std::shared_ptr<TileCache> cache, const Config& config)
    : cache_(cache), config_(config) {
}

AsyncTileLoader::~AsyncTileLoader() {
    stop();
}

void AsyncTileLoader::start() {
    if (running_.load()) {
        return;
    }
    
    running_.store(true);
    stopping_.store(false);
    
    workers_.clear();
    workers_.reserve(config_.numWorkerThreads);
    
    for (int i = 0; i < config_.numWorkerThreads; ++i) {
        workers_.emplace_back(&AsyncTileLoader::workerThread, this);
    }
}

void AsyncTileLoader::stop() {
    if (!running_.load()) {
        return;
    }
    
    stopping_.store(true);
    running_.store(false);
    
    queueCondition_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
}

std::future<LoadResult> AsyncTileLoader::loadTileAsync(const std::string& tileId, 
                                                      const std::string& resourceDir,
                                                      const TileMeta& tileMeta, 
                                                      int priority) {
    auto cachedTile = cache_->get(tileId);
    if (cachedTile) {
        stats_.cacheHits++;
        
        LoadResult result;
        result.tileId = tileId;
        result.status = LoadStatus::Completed;
        result.isPureColor = cachedTile->isPureColor;
        result.pureColorValue = cachedTile->pureColorValue;
        result.width = cachedTile->width;
        result.height = cachedTile->height;
        result.channels = cachedTile->channels;
        
        if (!cachedTile->isPureColor) {
            result.data = cachedTile->data;
        }
        
        auto promise = std::make_shared<std::promise<LoadResult>>();
        promise->set_value(std::move(result));
        return promise->get_future();
    }
    
    if (priority == -1) {
        priority = config_.defaultPriority;
    }
    
    auto promise = std::make_shared<std::promise<LoadResult>>();
    auto future = promise->get_future();
    
    std::string filePath = resourceDir + "/" + tileMeta.file;
    bool isPure = isPureColorTile(tileMeta.file);
    uint32_t color = isPure ? parseColorFromFileName(tileMeta.file) : 0;
    
    TileLoadRequest request(tileId, filePath, priority, isPure, color, tileMeta.w, tileMeta.h);
    
    {
        std::unique_lock<std::mutex> lock(callbackMutex_);
        callbacks_[tileId].push_back([promise](const LoadResult& result) {
            promise->set_value(result);
        });
    }
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (loadQueue_.size() < config_.maxQueueSize) {
            loadQueue_.push(request);
            stats_.queuedRequests++;
            updateStatus(tileId, LoadStatus::Pending);
        }
    }
    
    queueCondition_.notify_one();
    stats_.totalRequests++;
    
    return future;
}

void AsyncTileLoader::loadTileAsync(const std::string& tileId, 
                                   const std::string& resourceDir,
                                   const TileMeta& tileMeta, 
                                   LoadCallback callback,
                                   int priority) {
    auto cachedTile = cache_->get(tileId);
    if (cachedTile) {
        stats_.cacheHits++;
        
        LoadResult result;
        result.tileId = tileId;
        result.status = LoadStatus::Completed;
        result.isPureColor = cachedTile->isPureColor;
        result.pureColorValue = cachedTile->pureColorValue;
        result.width = cachedTile->width;
        result.height = cachedTile->height;
        result.channels = cachedTile->channels;
        
        if (!cachedTile->isPureColor) {
            result.data = cachedTile->data;
        }
        
        callback(result);
        return;
    }
    
    if (priority == -1) {
        priority = config_.defaultPriority;
    }
    
    std::string filePath = resourceDir + "/" + tileMeta.file;
    bool isPure = isPureColorTile(tileMeta.file);
    uint32_t color = isPure ? parseColorFromFileName(tileMeta.file) : 0;
    
    TileLoadRequest request(tileId, filePath, priority, isPure, color, tileMeta.w, tileMeta.h);
    
    {
        std::unique_lock<std::mutex> lock(callbackMutex_);
        callbacks_[tileId].push_back(callback);
    }
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (loadQueue_.size() < config_.maxQueueSize) {
            loadQueue_.push(request);
            stats_.queuedRequests++;
            updateStatus(tileId, LoadStatus::Pending);
        }
    }
    
    queueCondition_.notify_one();
    stats_.totalRequests++;
}

void AsyncTileLoader::preloadViewportTiles(const std::vector<TileMeta>& tiles, 
                                          const std::string& resourceDir,
                                          int basePriority) {
    if (!config_.enablePreloading) {
        return;
    }
    
    for (const auto& tileMeta : tiles) {
        std::string tileId = tileMeta.file;
        
        if (cache_->get(tileId) || isLoading(tileId)) {
            continue;
        }
        
        std::string filePath = resourceDir + "/" + tileMeta.file;
        bool isPure = isPureColorTile(tileMeta.file);
        uint32_t color = isPure ? parseColorFromFileName(tileMeta.file) : 0;
        
        TileLoadRequest request(tileId, filePath, basePriority, isPure, color, tileMeta.w, tileMeta.h);
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (loadQueue_.size() < config_.maxQueueSize) {
                loadQueue_.push(request);
                stats_.queuedRequests++;
                updateStatus(tileId, LoadStatus::Pending);
            }
        }
    }
    
    queueCondition_.notify_all();
}

void AsyncTileLoader::preloadByDirection(const Viewport& currentViewport, 
                                        const Viewport& movement,
                                        const TileIndex& index,
                                        const std::string& resourceDir) {
    if (!config_.enablePreloading) {
        return;
    }
    
    int expandX = std::abs(movement.x) + currentViewport.w / 2;
    int expandY = std::abs(movement.y) + currentViewport.h / 2;
    
    Viewport expandedViewport{
        currentViewport.x - expandX,
        currentViewport.y - expandY,
        currentViewport.w + 2 * expandX,
        currentViewport.h + 2 * expandY
    };
    
    auto tiles = index.query(expandedViewport);
    preloadViewportTiles(tiles, resourceDir, 25);
}

void AsyncTileLoader::cancelPendingRequests() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    std::priority_queue<TileLoadRequest> emptyQueue;
    std::swap(loadQueue_, emptyQueue);
    
    stats_.queuedRequests = 0;
}

void AsyncTileLoader::setPriorityBoost(const std::vector<std::string>& tileIds, int priorityBoost) {
}

AsyncTileLoader::Statistics AsyncTileLoader::getStatistics() const {
    Statistics result;
    result.totalRequests = stats_.totalRequests;
    result.completedLoads = stats_.completedLoads;
    result.failedLoads = stats_.failedLoads;
    result.cacheHits = stats_.cacheHits;
    result.queuedRequests = stats_.queuedRequests;
    result.activeLoads = stats_.activeLoads;
    return result;
}

bool AsyncTileLoader::isLoading(const std::string& tileId) const {
    std::unique_lock<std::mutex> lock(statusMutex_);
    auto it = loadStatus_.find(tileId);
    return it != loadStatus_.end() && 
           (it->second == LoadStatus::Pending || it->second == LoadStatus::Loading);
}

size_t AsyncTileLoader::getQueueSize() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return loadQueue_.size();
}

void AsyncTileLoader::workerThread() {
    while (running_.load()) {
        TileLoadRequest request("", "", 0);
        bool hasRequest = false;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this] { 
                return !loadQueue_.empty() || !running_.load(); 
            });
            
            if (!running_.load()) {
                break;
            }
            
            if (!loadQueue_.empty()) {
                request = loadQueue_.top();
                loadQueue_.pop();
                hasRequest = true;
                stats_.queuedRequests--;
                stats_.activeLoads++;
            }
        }
        
        if (hasRequest) {
            updateStatus(request.tileId, LoadStatus::Loading);
            
            LoadResult result = loadTileSync(request);
            
            if (result.status == LoadStatus::Completed) {
                if (result.isPureColor) {
                    cache_->putPureColor(result.tileId, result.pureColorValue, 
                                        result.width, result.height);
                } else {
                    cache_->put(result.tileId, std::move(result.data), 
                               result.width, result.height, result.channels);
                }
                stats_.completedLoads++;
            } else {
                stats_.failedLoads++;
            }
            
            updateStatus(request.tileId, result.status);
            notifyCallbacks(result);
            stats_.activeLoads--;
        }
    }
}

LoadResult AsyncTileLoader::loadTileSync(const TileLoadRequest& request) {
    LoadResult result;
    result.tileId = request.tileId;
    
    try {
        if (request.isPureColor) {
            result = createPureColorTile(request.tileId, request.pureColorValue, 
                                        request.width, request.height);
        } else {
            result = loadImageTile(request.filePath);
            result.tileId = request.tileId;
        }
    } catch (const std::exception& e) {
        result.status = LoadStatus::Failed;
        result.error = e.what();
    }
    
    return result;
}

LoadResult AsyncTileLoader::loadImageTile(const std::string& filePath) {
    LoadResult result;
    
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        result.status = LoadStatus::Failed;
        result.error = "Failed to load image: " + filePath;
        return result;
    }
    
    result.status = LoadStatus::Completed;
    result.width = width;
    result.height = height;
    result.channels = 4;
    result.isPureColor = false;
    
    size_t dataSize = width * height * 4;
    result.data.resize(dataSize);
    std::memcpy(result.data.data(), data, dataSize);
    
    stbi_image_free(data);
    
    return result;
}

LoadResult AsyncTileLoader::createPureColorTile(const std::string& tileId, uint32_t color, int width, int height) {
    LoadResult result;
    result.tileId = tileId;
    result.status = LoadStatus::Completed;
    result.width = width;
    result.height = height;
    result.channels = 4;
    result.isPureColor = true;
    result.pureColorValue = color;
    
    return result;
}

void AsyncTileLoader::notifyCallbacks(const LoadResult& result) {
    std::vector<LoadCallback> callbacksCopy;
    
    {
        std::unique_lock<std::mutex> lock(callbackMutex_);
        auto it = callbacks_.find(result.tileId);
        if (it != callbacks_.end()) {
            callbacksCopy = std::move(it->second);
            callbacks_.erase(it);
        }
    }
    
    for (const auto& callback : callbacksCopy) {
        try {
            callback(result);
        } catch (const std::exception& e) {
            std::cerr << "Callback error for tile " << result.tileId << ": " << e.what() << std::endl;
        }
    }
}

void AsyncTileLoader::updateStatus(const std::string& tileId, LoadStatus status) {
    std::unique_lock<std::mutex> lock(statusMutex_);
    loadStatus_[tileId] = status;
}

int AsyncTileLoader::calculatePriority(const TileMeta& tileMeta, const Viewport& viewport, int basePriority) const {
    int centerX = viewport.x + viewport.w / 2;
    int centerY = viewport.y + viewport.h / 2;
    
    int tileCenterX = tileMeta.x + tileMeta.w / 2;
    int tileCenterY = tileMeta.y + tileMeta.h / 2;
    
    double distance = std::sqrt(
        std::pow(tileCenterX - centerX, 2) + 
        std::pow(tileCenterY - centerY, 2)
    );
    
    return basePriority + static_cast<int>(distance / 10);
}

bool AsyncTileLoader::isPureColorTile(const std::string& fileName) const {
    return fileName.length() == 8;
}

uint32_t AsyncTileLoader::parseColorFromFileName(const std::string& fileName) const {
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