#include <iostream>

#include "QuadTreeSplitter.hpp"

/**
 * @brief 测试四叉树分割功能
 */
int main() {
    std::cout << "=== 四叉树分割测试 ===" << std::endl;

    QuadTreeSplitter splitter;

    // 配置分割参数
    QuadTreeSplitter::Config config;
    config.maxDepth = 8;        // 最大深度
    config.minTileSize = 8;     // 最小瓦片尺寸
    config.colorTolerance = 0;  // 严格颜色匹配

    std::cout << "配置参数:" << std::endl;
    std::cout << "  最大深度: " << config.maxDepth << std::endl;
    std::cout << "  最小瓦片尺寸: " << config.minTileSize << std::endl;
    std::cout << "  颜色容差: " << config.colorTolerance << std::endl;

    // 测试图像路径
    std::string inputPath = "data/sample1.png";
    std::string outputDir = "data/quadtree_tiles";

    std::cout << "\n开始四叉树分割..." << std::endl;
    std::cout << "输入图像: " << inputPath << std::endl;
    std::cout << "输出目录: " << outputDir << std::endl;

    // 执行四叉树分割
    auto tiles = splitter.splitQuadTree(inputPath, outputDir, config);

    std::cout << "\n分割结果:" << std::endl;
    std::cout << "生成瓦片数量: " << tiles.size() << std::endl;

    // 显示前几个瓦片的信息
    int displayCount = std::min(10, (int)tiles.size());
    std::cout << "\n前 " << displayCount << " 个瓦片信息:" << std::endl;
    for (int i = 0; i < displayCount; ++i) {
        const auto& tile = tiles[i];
        std::cout << "  瓦片 " << i + 1 << ": (" << tile.x << ", " << tile.y
                  << ") " << tile.w << "x" << tile.h << " -> " << tile.file
                  << std::endl;
    }

    if (tiles.size() > displayCount) {
        std::cout << "  ... 还有 " << (tiles.size() - displayCount) << " 个瓦片"
                  << std::endl;
    }

    std::cout << "\n测试完成!" << std::endl;
    return 0;
}
