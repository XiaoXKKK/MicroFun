# MicroFun 地图加载优化项目开发指导

## 项目概述

MicroFun 是一个专注于**客户端地图加载优化**的研究项目，旨在解决游戏地图资源庞大导致的显存占用高、加载速度慢问题。项目通过设计高效的地图资源拆分与组合方案，实现更灵活精细的拆分算法与组合机制。

## 核心架构与组件

### 主要模块
- **TileSplitter**: 地图瓦片切分器，基于 stb_image 库处理 PNG 图片
- **TileIndex**: 瓦片索引管理，提供视口 AABB 查询功能
- **ViewportAssembler**: 视口组装器，通过 alpha 混合组合瓦片生成最终图片

### 项目结构
```
MicroFun/
├── run.sh              # 统一入口脚本
├── CMakeLists.txt      # 顶层构建配置
├── lib/                # 核心库代码
│   ├── include/        # 头文件
│   └── src/            # 源码实现
├── tools/              # 工具程序入口
├── data/               # 示例数据和输出
└── bin/                # 编译产物
```

## 开发规范与最佳实践

### 代码规范
1. **命名约定**:
   - 类名使用 PascalCase (如 `TileSplitter`)
   - 函数名和变量名使用 camelCase (如 `splitTiles`)
   - 常量使用 UPPER_SNAKE_CASE

2. **文件组织**:
   - 头文件放在 `lib/include/` 目录
   - 实现文件放在 `lib/src/` 目录
   - 工具程序入口放在 `tools/` 目录

3. **依赖管理**:
   - 使用 stb_image 库处理图像 I/O
   - 依赖项通过 CMake 管理
   - 避免引入不必要的外部依赖

### 性能考虑
1. **内存管理**:
   - 及时释放图像数据 (`stbi_image_free`)
   - 使用 RAII 原则管理资源
   - 避免内存泄漏和重复分配

2. **算法优化**:
   - 使用空间索引结构优化查询 (考虑 QuadTree/R-Tree)
   - 实现高效的 alpha 混合算法
   - 优化瓦片加载和缓存策略

## 功能开发指导

### 当前功能
- ✅ PNG 图片的瓦片切分
- ✅ 瓦片索引管理和查询
- ✅ 视口区域的瓦片组装
- ✅ 命令行工具接口

### 优先开发方向

#### 1. 自适应瓦片粒度 (高优先级)
```cpp
// 建议实现位置: lib/src/AdaptiveTileSplitter.cpp
class AdaptiveTileSplitter {
    // 基于图像内容复杂度动态调整瓦片大小
    std::vector<TileMeta> splitAdaptive(const std::string& input, 
                                       float complexityThreshold = 0.5f);
};
```

#### 2. 分层索引结构 (高优先级)
```cpp
// 建议实现位置: lib/src/SpatialIndex.cpp
class QuadTreeIndex : public TileIndex {
    // 实现四叉树空间索引
    std::vector<TileMeta> queryRange(const BoundingBox& bbox) const override;
};
```

#### 3. 缓存管理系统 (中优先级)
```cpp
// 建议实现位置: lib/src/TileCache.cpp
class TileCache {
    // LRU + LFU 混合缓存策略
    void setMemoryBudget(size_t maxBytes);
    TileData* getTile(const std::string& tileId);
    void evictOldTiles();
};
```

#### 4. 异步加载机制 (中优先级)
```cpp
// 建议实现位置: lib/src/AsyncLoader.cpp
class AsyncTileLoader {
    // 线程池 + 队列实现异步加载
    std::future<TileData> loadTileAsync(const std::string& tilePath);
    void preloadTiles(const std::vector<std::string>& tilePaths);
};
```

### 新增模块建议

#### scheduler/ 目录
- `TileScheduler.hpp/cpp`: 瓦片优先级调度
- `LoadPolicy.hpp/cpp`: 加载策略定义
- `PreloadManager.hpp/cpp`: 预加载管理器

#### cache/ 目录
- `CachePolicy.hpp/cpp`: 缓存策略接口
- `LRUCache.hpp/cpp`: LRU 缓存实现
- `MemoryBudget.hpp/cpp`: 显存预算管理

#### io/ 目录
- `AsyncIO.hpp/cpp`: 异步 I/O 操作
- `Decompressor.hpp/cpp`: 图像解压缩
- `TexturePacker.hpp/cpp`: 纹理打包器

## 测试与性能评估

### 测试用例建议
1. **功能测试**:
   - 不同尺寸图片的切分正确性
   - 边界条件的视口查询
   - 空瓦片和重叠瓦片的处理

2. **性能测试**:
   - 大图切分时间测试
   - 内存占用监控
   - 并发加载性能测试

### 性能指标收集
```cpp
// 建议实现位置: lib/src/PerformanceMonitor.cpp
class PerformanceMonitor {
    void recordSplitTime(const std::string& imageFile, double milliseconds);
    void recordMemoryUsage(size_t bytes);
    void recordCacheHitRate(float hitRate);
    void exportMetrics(const std::string& outputFile);
};
```

## 命令行接口扩展

### 当前命令 (已更新)
- 预处理拆分: `./run.sh split -i <input.png> -o <output_dir>`  
   - 生成 `<output_dir>` 下所有瓦片 PNG 与 `meta.txt` 索引文件。
   - 内部仍支持隐藏参数 `--tile WxH` 与 `--meta meta.txt` 以便调试；正式文档不再暴露。
- 视口组合预览: `./run.sh run -i <resource_dir> -p <posx>,<posy> -s <w>,<h>`  
   - 默认标准输出按行主序输出 `w*h` 个像素的 `0xRRGGBBAA`（逗号分隔）。
   - 可选 `-o viewport.png` 输出 PNG（兼容旧流程）。
- 向后兼容: `run` == `check`，保留旧命令名。

示例:
```bash
./run.sh split -i data/sample1.png -o data/tiles
./run.sh run -i data/tiles -p 0,0 -s 128,128            # 输出十六进制像素流
./run.sh run -i data/tiles -p 0,0 -s 512,512 -o vp.png  # 输出 PNG
```

### 建议新增命令（保持不变，待实现）
```bash
# 性能分析
./run.sh benchmark -i input.png --iterations 10 --report bench.json

# 缓存管理
./run.sh cache --budget 512MB --policy lru --stats

# 批量处理
./run.sh batch -i images/ -o tiles/ --config batch.json

# 可视化工具
./run.sh visualize -i meta.txt --heatmap --output viz.png
```

## 文档要求

### 代码注释
- 所有公共接口必须有详细的 Doxygen 注释
- 复杂算法需要有算法思路说明
- 性能关键代码需要有优化说明

### 提交信息
- 使用规范的 commit message 格式
- 功能开发: `feat: 添加自适应瓦片切分功能`
- 性能优化: `perf: 优化视口查询算法，提升50%性能`
- 修复问题: `fix: 修复边界瓦片索引越界问题`

## 平台兼容性

### 目标环境
- **操作系统**: CentOS 7.9.2009 x86_64
- **编译器**: GCC 9.3.1 / Clang 14.0.4
- **硬件**: Intel i7-11800H, 16GB RAM

### 兼容性要求
- 使用 C++17 标准
- 避免使用平台特定 API
- 确保在目标环境下的构建成功

## 调试与问题排查

### 常见问题
1. **图像加载失败**: 检查 stb_image 错误信息
2. **内存泄漏**: 使用 valgrind 检测
3. **性能瓶颈**: 使用 profiler 分析热点

### 调试工具
- 使用 `gdb` 进行断点调试
- 使用 `valgrind` 检测内存问题
- 使用 `perf` 进行性能分析

## 贡献指南

### 开发流程
1. 创建功能分支: `git checkout -b feature/adaptive-tiles`
2. 实现功能并添加测试
3. 更新相关文档
4. 提交 PR 并通过 code review

### 代码审查要点
- 功能完整性和正确性
- 性能优化和内存安全
- 代码可读性和维护性
- 测试覆盖率和文档完整性

---

## 快速开始开发

1. **设置开发环境**:
   ```bash
   git clone <repository>
   cd MicroFun
   ./run.sh split -i data/sample1.png --tile 32x32
   ```

2. **添加新功能**:
   - 在 `lib/include/` 添加头文件
   - 在 `lib/src/` 添加实现
   - 更新 `CMakeLists.txt`
   - 添加相应测试

3. **性能测试**:
   ```bash
   # 编译 Release 版本
   cmake -DCMAKE_BUILD_TYPE=Release ..
   # 运行性能测试
   ./benchmark_script.sh
   ```

记住：始终以性能优化和用户体验为核心目标，保持代码简洁高效！
