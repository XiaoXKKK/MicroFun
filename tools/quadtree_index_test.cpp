#include <chrono>
#include <iostream>

#include "QuadTreeIndex.hpp"
#include "TileIndex.hpp"

using namespace std;
using namespace chrono;

int main() {
    cout << "=== QuadTreeIndex 测试程序 ===" << endl;

    string metaFile = "data/test_quadtree/meta.txt";

    // 测试普通TileIndex
    cout << "\n1. 测试普通 TileIndex:" << endl;
    TileIndex normalIndex;
    auto start = high_resolution_clock::now();
    bool normalLoaded = normalIndex.load(metaFile);
    auto end = high_resolution_clock::now();
    auto normalLoadTime = duration_cast<microseconds>(end - start).count();

    if (!normalLoaded) {
        cerr << "无法加载 meta 文件: " << metaFile << endl;
        return 1;
    }

    cout << "  - 加载时间: " << normalLoadTime << " 微秒" << endl;
    cout << "  - 地图尺寸: " << normalIndex.getMapWidth() << "x"
         << normalIndex.getMapHeight() << endl;

    // 测试QuadTreeIndex
    cout << "\n2. 测试 QuadTreeIndex:" << endl;
    QuadTreeIndex qtIndex;
    start = high_resolution_clock::now();
    bool qtLoaded = qtIndex.load(metaFile);
    end = high_resolution_clock::now();
    auto qtLoadTime = duration_cast<microseconds>(end - start).count();

    if (!qtLoaded) {
        cerr << "QuadTreeIndex 加载失败" << endl;
        return 1;
    }

    cout << "  - 加载时间: " << qtLoadTime << " 微秒" << endl;
    cout << "  - 地图尺寸: " << qtIndex.getMapWidth() << "x"
         << qtIndex.getMapHeight() << endl;

    // 获取四叉树统计信息
    auto stats = qtIndex.getStatistics();
    cout << "  - 四叉树统计:" << endl;
    cout << "    * 总节点数: " << stats.totalNodes << endl;
    cout << "    * 叶子节点数: " << stats.leafNodes << endl;
    cout << "    * 最大深度: " << stats.maxDepth << endl;
    cout << "    * 总瓦片数: " << stats.totalTiles << endl;
    cout << "    * 平均每叶子节点瓦片数: " << stats.avgTilesPerLeaf << endl;

    // 测试查询性能
    cout << "\n3. 查询性能对比:" << endl;

    // 测试不同尺寸的视口
    vector<Viewport> testViewports = {
        {0, 0, 128, 128},      // 小视口
        {100, 100, 256, 256},  // 中等视口
        {0, 0, 512, 512},      // 大视口
        {200, 200, 64, 64}     // 很小的视口
    };

    for (const auto& vp : testViewports) {
        cout << "\n  视口 [" << vp.x << "," << vp.y << " " << vp.w << "x"
             << vp.h << "]:" << endl;

        // 普通索引查询
        start = high_resolution_clock::now();
        auto normalResult = normalIndex.query(vp);
        end = high_resolution_clock::now();
        auto normalQueryTime = duration_cast<microseconds>(end - start).count();

        // 四叉树索引查询
        start = high_resolution_clock::now();
        auto qtResult = qtIndex.query(vp);
        end = high_resolution_clock::now();
        auto qtQueryTime = duration_cast<microseconds>(end - start).count();

        cout << "    - 普通索引: " << normalQueryTime << " 微秒, "
             << normalResult.size() << " 瓦片" << endl;
        cout << "    - 四叉树索引: " << qtQueryTime << " 微秒, "
             << qtResult.size() << " 瓦片" << endl;

        if (normalQueryTime > 0) {
            double speedup = static_cast<double>(normalQueryTime) / qtQueryTime;
            cout << "    - 加速比: " << speedup << "x" << endl;
        }

        // 验证结果一致性
        if (normalResult.size() != qtResult.size()) {
            cout << "    - ⚠️ 警告: 结果数量不一致!" << endl;
        } else {
            cout << "    - ✅ 结果一致" << endl;
        }
    }

    cout << "\n=== 测试完成 ===" << endl;
    return 0;
}
