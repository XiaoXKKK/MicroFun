#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "TileCache.hpp"
#include "TileIndex.hpp"

struct TileLoadRequest {
    std::string tileId;
    std::string filePath;
    int priority;
    bool isPureColor;
    uint32_t pureColorValue;
    int width;
    int height;
    
    TileLoadRequest(const std::string& id, const std::string& path, int prio, 
                   bool isPure = false, uint32_t color = 0, int w = 0, int h = 0)
        : tileId(id), filePath(path), priority(prio), isPureColor(isPure), 
          pureColorValue(color), width(w), height(h) {}
    
    bool operator<(const TileLoadRequest& other) const {
        return priority < other.priority;
    }
};

enum class LoadStatus {
    Pending,
    Loading,
    Completed,
    Failed
};

struct LoadResult {
    std::string tileId;
    LoadStatus status;
    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool isPureColor = false;
    uint32_t pureColorValue = 0;
    std::string error;
};

class AsyncTileLoader {
public:
    struct Config {
        int numWorkerThreads;
        size_t maxQueueSize;
        int defaultPriority;
        bool enablePreloading;
        
        Config() : numWorkerThreads(4), maxQueueSize(1000), 
                  defaultPriority(100), enablePreloading(true) {}
    };
    
    struct Statistics {
        size_t totalRequests = 0;
        size_t completedLoads = 0;
        size_t failedLoads = 0;
        size_t cacheHits = 0;
        size_t queuedRequests = 0;
        size_t activeLoads = 0;
        
        double getSuccessRate() const {
            size_t total = completedLoads + failedLoads;
            if (total == 0) return 0.0;
            return static_cast<double>(completedLoads) / total;
        }
    };
    
    using LoadCallback = std::function<void(const LoadResult&)>;
    
    explicit AsyncTileLoader(std::shared_ptr<TileCache> cache, const Config& config = Config());
    ~AsyncTileLoader();
    
    AsyncTileLoader(const AsyncTileLoader&) = delete;
    AsyncTileLoader& operator=(const AsyncTileLoader&) = delete;
    
    void start();
    
    void stop();
    
    std::future<LoadResult> loadTileAsync(const std::string& tileId, 
                                         const std::string& resourceDir,
                                         const TileMeta& tileMeta, 
                                         int priority = -1);
    
    void loadTileAsync(const std::string& tileId, 
                      const std::string& resourceDir,
                      const TileMeta& tileMeta, 
                      LoadCallback callback,
                      int priority = -1);
    
    void preloadViewportTiles(const std::vector<TileMeta>& tiles, 
                             const std::string& resourceDir,
                             int basePriority = 50);
    
    void preloadByDirection(const Viewport& currentViewport, 
                           const Viewport& movement,
                           const TileIndex& index,
                           const std::string& resourceDir);
    
    void cancelPendingRequests();
    
    void setPriorityBoost(const std::vector<std::string>& tileIds, int priorityBoost);
    
    Statistics getStatistics() const;
    
    bool isLoading(const std::string& tileId) const;
    
    size_t getQueueSize() const;

private:
    std::shared_ptr<TileCache> cache_;
    Config config_;
    
    mutable Statistics stats_;
    
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopping_{false};
    
    mutable std::mutex queueMutex_;
    std::priority_queue<TileLoadRequest> loadQueue_;
    std::condition_variable queueCondition_;
    
    mutable std::mutex callbackMutex_;
    std::unordered_map<std::string, std::vector<LoadCallback>> callbacks_;
    
    mutable std::mutex statusMutex_;
    std::unordered_map<std::string, LoadStatus> loadStatus_;
    
    void workerThread();
    
    LoadResult loadTileSync(const TileLoadRequest& request);
    
    LoadResult loadImageTile(const std::string& filePath);
    
    LoadResult createPureColorTile(const std::string& tileId, uint32_t color, int width, int height);
    
    void notifyCallbacks(const LoadResult& result);
    
    void updateStatus(const std::string& tileId, LoadStatus status);
    
    int calculatePriority(const TileMeta& tileMeta, const Viewport& viewport, int basePriority) const;
    
    bool isPureColorTile(const std::string& fileName) const;
    
    uint32_t parseColorFromFileName(const std::string& fileName) const;
};