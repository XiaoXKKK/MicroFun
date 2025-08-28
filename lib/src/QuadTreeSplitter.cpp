#include "QuadTreeSplitter.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>

#include "stb_image.h"
#include "stb_image_write.h"

QuadTreeSplitter::QuadTreeSplitter() {
    // 构造函数，初始化颜色检查器
}

std::vector<TileMeta> QuadTreeSplitter::splitQuadTree(
    const std::string& inputPath, const std::string& outDir,
    const Config& config) {
    std::vector<TileMeta> tiles;

    // 加载图像
    int width, height, channels;
    unsigned char* imageData =
        stbi_load(inputPath.c_str(), &width, &height, &channels, 4);
    if (!imageData) {
        std::cerr << "Failed to load image: " << inputPath << std::endl;
        return tiles;
    }

    std::cout << "Loaded image: " << width << "x" << height << " (" << channels
              << " channels)" << std::endl;

    // 确保输出目录存在
    if (!ensureDirectoryExists(outDir)) {
        std::cerr << "Failed to create output directory: " << outDir
                  << std::endl;
        stbi_image_free(imageData);
        return tiles;
    }

    // 设置颜色检查器的容差
    colorChecker_.setColorTolerance(config.colorTolerance);

    // 构建四叉树
    auto quadTree = buildQuadTree(imageData, width, height, config);
    if (!quadTree) {
        std::cerr << "Failed to build quad tree" << std::endl;
        stbi_image_free(imageData);
        return tiles;
    }

    // 收集叶子节点并生成瓦片
    collectLeafTiles(quadTree.get(), imageData, width, height, outDir, tiles);

    // 释放图像数据
    stbi_image_free(imageData);

    std::cout << "QuadTree split completed: " << tiles.size()
              << " tiles generated" << std::endl;
    return tiles;
}

std::vector<TileMeta> QuadTreeSplitter::split(const std::string& inputPath,
                                              const std::string& outDir,
                                              int tileW, int tileH) {
    // 直接使用四叉树分割，不使用兼容模式
    Config config;
    config.minTileSize = std::min(tileW, tileH);
    return splitQuadTree(inputPath, outDir, config);
}

std::unique_ptr<QuadTreeNode> QuadTreeSplitter::buildQuadTree(
    const unsigned char* imageData, int imageWidth, int imageHeight,
    const Config& config) {
    std::cout << "Building quad tree with size: " << imageWidth << "x"
              << imageHeight << std::endl;

    // 创建根节点，覆盖整个图像
    auto root = std::make_unique<QuadTreeNode>(0, 0, imageWidth, imageHeight);

    // 递归分割
    subdivideNode(root.get(), imageData, imageWidth, imageHeight, config, 0);

    return root;
}

void QuadTreeSplitter::subdivideNode(QuadTreeNode* node,
                                     const unsigned char* imageData,
                                     int imageWidth, int imageHeight,
                                     const Config& config, int currentDepth) {
    // 检查边界条件
    if (node->getX() >= imageWidth || node->getY() >= imageHeight) {
        return;
    }

    // 计算实际可用的区域尺寸
    int actualWidth = std::min(node->getWidth(), imageWidth - node->getX());
    int actualHeight = std::min(node->getHeight(), imageHeight - node->getY());

    // 检查终止条件
    uint32_t uniformColor;
    bool isUniform = colorChecker_.isUniformColor(
        imageData, imageWidth, node->getX(), node->getY(), actualWidth,
        actualHeight, uniformColor);

    // 终止条件：1. 颜色一致 2. 达到最大深度 3. 达到最小尺寸
    if (isUniform || currentDepth >= config.maxDepth ||
        actualWidth <= config.minTileSize ||
        actualHeight <= config.minTileSize) {
        // 标记为叶子节点
        if (isUniform) {
            node->setUniformColor(uniformColor);
            node->setHasUniformColor(true);
        }
        return;
    }

    // 只有当区域足够大时才进行分割
    if (actualWidth > 1 && actualHeight > 1) {
        // 四等分并递归处理
        node->subdivide();
        for (const auto& child : node->getChildren()) {
            subdivideNode(child.get(), imageData, imageWidth, imageHeight,
                          config, currentDepth + 1);
        }
    }
}

void QuadTreeSplitter::collectLeafTiles(const QuadTreeNode* node,
                                        const unsigned char* imageData,
                                        int imageWidth, int imageHeight,
                                        const std::string& outDir,
                                        std::vector<TileMeta>& tiles) {
    if (node->isLeaf()) {
        // 叶子节点，生成瓦片
        int x = node->getX();
        int y = node->getY();
        int width = node->getWidth();
        int height = node->getHeight();

        // 检查边界
        if (x >= imageWidth || y >= imageHeight) {
            return;
        }

        // 计算实际尺寸
        int actualWidth = std::min(width, imageWidth - x);
        int actualHeight = std::min(height, imageHeight - y);

        if (actualWidth <= 0 || actualHeight <= 0) {
            return;
        }

        // 生成瓦片文件名和处理纯色瓦片
        std::string fileName;
        bool success = false;

        if (node->hasUniformColor()) {
            // 纯色瓦片：使用RGBA十六进制值作为文件名
            uint32_t color = node->getUniformColor();
            char hexColor[10];
            snprintf(hexColor, sizeof(hexColor), "%08X", color);
            fileName = std::string(hexColor);

            // 对于纯色瓦片，不需要生成实际的PNG文件
            success = true;
        } else {
            // 混合颜色瓦片：生成正常的瓦片文件
            fileName = generateTileFileName(x, y, width, height);
            std::string filePath = outDir + "/" + fileName;

            success = generateTile(imageData, imageWidth, imageHeight, x, y,
                                   width, height, filePath);
        }

        if (success) {
            // 添加到瓦片元数据
            TileMeta meta;
            meta.x = x;
            meta.y = y;
            meta.w = actualWidth;
            meta.h = actualHeight;
            meta.file = fileName;
            tiles.push_back(meta);

            // 输出调试信息
            if (node->hasUniformColor()) {
                std::cout << "Pure color tile: (" << x << "," << y << ") "
                          << actualWidth << "x" << actualHeight
                          << " -> color: " << fileName << std::endl;
            }
        }
    } else {
        // 内部节点，递归处理子节点
        for (const auto& child : node->getChildren()) {
            collectLeafTiles(child.get(), imageData, imageWidth, imageHeight,
                             outDir, tiles);
        }
    }
}

bool QuadTreeSplitter::generateTile(const unsigned char* imageData,
                                    int imageWidth, int imageHeight, int x,
                                    int y, int width, int height,
                                    const std::string& outputPath) {
    // 计算实际输出尺寸
    int actualWidth = std::min(width, imageWidth - x);
    int actualHeight = std::min(height, imageHeight - y);

    if (actualWidth <= 0 || actualHeight <= 0) {
        return false;
    }

    // 创建瓦片数据（只处理非纯色瓦片）
    std::vector<unsigned char> tileData(width * height * 4);

    // 从原图复制数据
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            int srcX = x + dx;
            int srcY = y + dy;

            if (srcX < imageWidth && srcY < imageHeight) {
                // 复制像素数据
                const unsigned char* srcPixel =
                    &imageData[(srcY * imageWidth + srcX) * 4];
                unsigned char* dstPixel = &tileData[(dy * width + dx) * 4];

                dstPixel[0] = srcPixel[0];  // R
                dstPixel[1] = srcPixel[1];  // G
                dstPixel[2] = srcPixel[2];  // B
                dstPixel[3] = srcPixel[3];  // A
            } else {
                // 超出边界，填充透明像素
                unsigned char* dstPixel = &tileData[(dy * width + dx) * 4];
                dstPixel[0] = 0;
                dstPixel[1] = 0;
                dstPixel[2] = 0;
                dstPixel[3] = 0;
            }
        }
    }

    // 保存PNG文件
    int result = stbi_write_png(outputPath.c_str(), width, height, 4,
                                tileData.data(), width * 4);
    return result != 0;
}

bool QuadTreeSplitter::ensureDirectoryExists(const std::string& outDir) {
    try {
        std::filesystem::create_directories(outDir);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory " << outDir << ": " << e.what()
                  << std::endl;
        return false;
    }
}

std::string QuadTreeSplitter::generateTileFileName(int x, int y, int width,
                                                   int height) const {
    return "qtile_" + std::to_string(x) + "_" + std::to_string(y) + "_" +
           std::to_string(width) + "x" + std::to_string(height) + ".png";
}

bool QuadTreeSplitter::isPureColorTile(const std::string& fileName) {
    // 纯色瓦片文件名格式：8位十六进制RGBA值（如 "FF0000FF"）
    if (fileName.length() != 8) {
        return false;
    }

    // 检查是否全为十六进制字符
    for (char c : fileName) {
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
              (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }

    return true;
}

uint32_t QuadTreeSplitter::parseColorFromFileName(const std::string& fileName) {
    if (!isPureColorTile(fileName)) {
        return 0;
    }

    // 将十六进制字符串转换为uint32_t
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
