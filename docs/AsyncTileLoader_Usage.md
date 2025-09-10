# AsyncTileLoader 使用说明

AsyncTileLoader 是 MicroFun 项目中的异步瓦片加载器，用于高效的多线程瓦片加载，支持缓存集成、优先级队列和预加载优化。

## 概述

AsyncTileLoader 提供了以下核心功能：
- 多线程异步瓦片加载
- 智能缓存集成，避免重复加载
- 基于优先级的任务调度
- 纯色瓦片优化处理
- 视口预加载和方向预测加载
- 完整的统计监控和错误处理

## 基本使用流程

### 1. 创建和配置

```cpp
#include "AsyncTileLoader.hpp"
#include "TileCache.hpp"

// 创建缓存
auto cache = std::make_shared<TileCache>(maxCacheSizeBytes);

// 配置加载器
AsyncTileLoader::Config config;
config.numWorkerThreads = 4;        // 工作线程数
config.maxQueueSize = 1000;         // 最大队列大小
config.defaultPriority = 100;       // 默认优先级
config.enablePreloading = true;     // 启用预加载

// 创建加载器
AsyncTileLoader loader(cache, config);
```

### 2. 启动和停止

```cpp
// 启动工作线程
loader.start();

// 使用加载器进行瓦片加载...

// 停止并等待所有线程完成
loader.stop();
```

### 3. 异步加载瓦片

#### 使用 Future 方式

```cpp
// 异步加载瓦片，返回 Future
auto future = loader.loadTileAsync(tileId, resourceDir, tileMeta, priority);

// 阻塞等待结果
LoadResult result = future.get();

if (result.status == LoadStatus::Completed) {
    if (result.isPureColor) {
        // 处理纯色瓦片
        uint32_t color = result.pureColorValue;
        // 使用颜色值...
    } else {
        // 处理图像瓦片
        std::vector<unsigned char>& imageData = result.data;
        int width = result.width;
        int height = result.height;
        int channels = result.channels;
        // 处理图像数据...
    }
} else if (result.status == LoadStatus::Failed) {
    std::cout << "加载失败: " << result.error << std::endl;
}
```

#### 使用回调方式

```cpp
loader.loadTileAsync(tileId, resourceDir, tileMeta, 
    [](const LoadResult& result) {
        if (result.status == LoadStatus::Completed) {
            if (result.isPureColor) {
                // 处理纯色瓦片
                handlePureColorTile(result.pureColorValue, result.width, result.height);
            } else {
                // 处理图像瓦片
                handleImageTile(result.data, result.width, result.height, result.channels);
            }
        } else {
            std::cout << "瓦片 " << result.tileId << " 加载失败" << std::endl;
        }
    }, priority);
```

## 高级功能

### 1. 预加载策略

#### 视口瓦片预加载

```cpp
// 获取当前视口内的瓦片
std::vector<TileMeta> tiles = index.query(viewport);

// 预加载视口内的瓦片
loader.preloadViewportTiles(tiles, resourceDir, basePriority);
```

#### 基于移动方向的预加载

```cpp
// 定义移动向量
Viewport movement{dx, dy, 0, 0};

// 基于移动方向预加载瓦片
loader.preloadByDirection(currentViewport, movement, index, resourceDir);
```

### 2. 任务管理

```cpp
// 检查瓦片是否正在加载
bool isLoading = loader.isLoading(tileId);

// 获取当前队列大小
size_t queueSize = loader.getQueueSize();

// 取消所有待处理的请求
loader.cancelPendingRequests();

// 设置优先级提升（功能预留，当前未实现）
std::vector<std::string> priorityTiles = {"tile1", "tile2"};
loader.setPriorityBoost(priorityTiles, 50);
```

### 3. 统计和监控

```cpp
// 获取加载统计信息
AsyncTileLoader::Statistics stats = loader.getStatistics();

std::cout << "总请求数: " << stats.totalRequests << std::endl;
std::cout << "完成加载: " << stats.completedLoads << std::endl;
std::cout << "失败加载: " << stats.failedLoads << std::endl;
std::cout << "缓存命中: " << stats.cacheHits << std::endl;
std::cout << "队列中请求: " << stats.queuedRequests << std::endl;
std::cout << "活跃加载: " << stats.activeLoads << std::endl;
std::cout << "成功率: " << stats.getSuccessRate() * 100 << "%" << std::endl;
```

## 数据结构说明

### LoadResult 结构

```cpp
struct LoadResult {
    std::string tileId;                     // 瓦片ID
    LoadStatus status;                      // 加载状态
    std::vector<unsigned char> data;        // 图像数据（非纯色瓦片）
    int width = 0;                          // 宽度
    int height = 0;                         // 高度
    int channels = 0;                       // 通道数
    bool isPureColor = false;               // 是否为纯色瓦片
    uint32_t pureColorValue = 0;            // 纯色值（纯色瓦片）
    std::string error;                      // 错误信息
};
```

### LoadStatus 枚举

```cpp
enum class LoadStatus {
    Pending,    // 待处理
    Loading,    // 加载中
    Completed,  // 已完成
    Failed      // 失败
};
```

### Config 配置结构

```cpp
struct Config {
    int numWorkerThreads = 4;       // 工作线程数量
    size_t maxQueueSize = 1000;     // 最大队列大小
    int defaultPriority = 100;      // 默认优先级
    bool enablePreloading = true;   // 是否启用预加载
};
```

## 核心特性

### 1. 缓存集成
- 自动检查 TileCache 中是否已有缓存
- 缓存命中时立即返回结果，无需重新加载
- 加载完成后自动将结果存入缓存

### 2. 纯色瓦片优化
- 8字符文件名被识别为纯色瓦片（如 "FFFFFFFF"）
- 纯色瓦片不占用存储空间，仅存储颜色值
- 支持 RGBA 格式的颜色编码

### 3. 优先级调度
- 基于优先级队列的任务调度
- 高优先级任务优先处理
- 支持动态优先级计算（基于视口中心距离）

### 4. 线程安全
- 所有公共接口都是线程安全的
- 使用原子变量和互斥锁保护共享资源
- 支持多线程并发访问

### 5. 错误处理
- 完整的异常捕获和错误状态管理
- 详细的错误信息记录
- 回调函数异常隔离

## 使用示例

### 完整示例：视口瓦片加载

```cpp
#include "AsyncTileLoader.hpp"
#include "TileCache.hpp"
#include "QuadTreeIndex.hpp"

int main() {
    // 创建组件
    auto cache = std::make_shared<TileCache>(100 * 1024 * 1024);  // 100MB缓存
    QuadTreeIndex index("data/quad_tiles");
    
    AsyncTileLoader::Config config;
    config.numWorkerThreads = 4;
    AsyncTileLoader loader(cache, config);
    
    // 启动加载器
    loader.start();
    
    // 定义视口
    Viewport viewport{100, 100, 800, 600};
    
    // 查询需要的瓦片
    auto tiles = index.query(viewport);
    
    // 异步加载所有瓦片
    std::vector<std::future<LoadResult>> futures;
    for (const auto& tileMeta : tiles) {
        auto future = loader.loadTileAsync(
            tileMeta.file, 
            "data/quad_tiles", 
            tileMeta, 
            100  // 优先级
        );
        futures.push_back(std::move(future));
    }
    
    // 等待所有瓦片加载完成
    for (auto& future : futures) {
        LoadResult result = future.get();
        if (result.status == LoadStatus::Completed) {
            std::cout << "瓦片 " << result.tileId << " 加载成功" << std::endl;
        }
    }
    
    // 停止加载器
    loader.stop();
    
    return 0;
}
```

## 性能优化建议

1. **合理设置线程数量**：通常设置为 CPU 核心数的 1-2 倍
2. **适当的队列大小**：避免内存占用过大，建议 500-2000
3. **使用预加载**：提前加载可能需要的瓦片
4. **监控统计信息**：定期检查缓存命中率和失败率
5. **及时清理队列**：在视口变化时取消不需要的请求

## 注意事项

1. 必须在使用前调用 `start()`，使用后调用 `stop()`
2. `stop()` 会等待所有工作线程完成，可能需要一些时间
3. 纯色瓦片文件名必须是8位十六进制字符
4. 所有路径都应该使用绝对路径或相对于工作目录的路径
5. 回调函数中的异常会被捕获并记录，不会影响其他操作