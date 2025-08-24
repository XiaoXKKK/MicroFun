
### 项目结构（样例实现骨架）
```
MicroFun/
  run.sh                # 统一入口：split / check
  CMakeLists.txt        # 顶层构建文件
  lib/
    CMakeLists.txt
    include/            # 头文件 (TileSplitter / TileIndex / ViewportAssembler)
    src/                # 源码实现
  tools/
    split_main.cpp      # split 子命令入口
    check_main.cpp      # check 子命令入口
  data/
    sample1.png         # 示例地图文件1
    sample2.png         # 示例地图文件2
    viewport.png        # 视口组合输出结果
    tiles/              # 切分后的瓦片目录
  bin/                  # 编译后工具（run.sh 自动生成）
  build/                # 临时构建目录（可忽略提交）
```

### 使用步骤（快速体验）
1. 切分示例地图：
```
./run.sh split -i data/sample1.png --tile 32x32 --meta data/tiles/meta.txt
```
2. 模拟视口组合：
```
./run.sh check -i data/tiles/meta.txt -p 0,0 -s 128x128 -o data/viewport.png
```
3. 查看结果：打开 `data/viewport.png` 图片文件

### 代码骨架说明
- 使用真实的 PNG 图片作为大地图输入；`TileSplitter` 基于 stb_image 库加载图片，依据固定瓦片尺寸切分并输出 PNG 格式的瓦片文件与 meta 索引。
- `TileIndex` 提供视口 AABB 查询，`ViewportAssembler` 加载相关瓦片并通过 alpha 混合组合成最终的视口 PNG 图片输出。
- 构建：首次运行 `run.sh` 时自动进行 CMake 构建并缓存于 `build/`。

### 后续可扩展方向（建议写入方案设计文档）
1. 自适应瓦片粒度（基于内容复杂度/熵/特征密度）。
2. 分层索引结构（QuadTree / R-Tree / Hilbert 排序 + 二分搜索）。
3. 瓦片优先级调度（视口中心距离 + 运动预测 + 预取半径）。
4. 显存预算管理（LRU + LFU 混合策略，冷数据延迟卸载 + 背景解压）。
5. 透明像素裁剪 + 打包（MaxRects / Skyline）生成复合纹理，减少 Bind/Draw 次数。
6. I/O 异步化（线程池 + 解码流水线 + 双缓冲提交）。
7. 性能监控埋点（拆分时间、加载延迟、命中率、显存峰值、GC 次数）。

### 指标采集思路（示例）
- 加载时间：统计从发起视口变更到所有高优先级瓦片 ready 的毫秒数。
- 显存占用：对已加载瓦片纹理尺寸求和 / 与原始整图对比比例。
- 预取命中率：预取瓦片中在下一窗口内被访问的数量 / 预取总数。
- 瓦片回收滞后：从瓦片离开卸载策略阈值到实际释放的时间差。

### 课程/考核提交提示
- 保留当前最小代码骨架，后续在 `lib/` 逐步迭代；新增模块建议：`scheduler/`, `cache/`, `io/`。
- 方案文档中建议附：流程时序图、数据结构图、性能对比（表+折线）。
- `run.sh check` 可扩展：支持路径脚本（移动轨迹）、统计输出 JSON、多种图片格式输出。 
