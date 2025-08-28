#ifndef QUADTREESPLITTER_HPP
#define QUADTREESPLITTER_HPP

#include "TileSplitter.hpp"
#include "QuadTreeNode.hpp"
#include "ColorChecker.hpp"
#include <memory>

/**
 * @brief 四叉树分割器类，基于颜色一致性的地图分割
 * 
 * 继承自 TileSplitter，提供四叉树分割功能，可以根据区域颜色一致性
 * 动态调整瓦片大小，优化存储效率。
 */
class QuadTreeSplitter : public TileSplitter {
public:
    /**
     * @brief 四叉树分割配置参数
     */
    struct Config {
        int maxDepth;           ///< 最大分割深度
        int minTileSize;        ///< 最小瓦片尺寸（像素）
        int colorTolerance;     ///< 颜色比较容差
        
        Config() : maxDepth(8), minTileSize(4), colorTolerance(0) {}
        Config(int depth, int minSize, int tolerance = 0) 
            : maxDepth(depth), minTileSize(minSize), colorTolerance(tolerance) {}
    };

    /**
     * @brief 构造函数
     */
    QuadTreeSplitter();

    /**
     * @brief 析构函数
     */
    ~QuadTreeSplitter() = default;

    /**
     * @brief 四叉树分割主接口
     * 
     * @param inputPath 输入图像文件路径
     * @param outDir 输出目录路径
     * @param config 分割配置参数
     * @return 生成的瓦片元数据列表
     */
    std::vector<TileMeta> splitQuadTree(
        const std::string& inputPath, 
        const std::string& outDir,
        const Config& config = Config{}
    );

    /**
     * @brief 兼容现有接口的分割方法
     * 
     * 当使用此接口时，将使用固定尺寸分割模式以保持向后兼容。
     * 
     * @param inputPath 输入图像文件路径
     * @param outDir 输出目录路径
     * @param tileW 瓦片宽度
     * @param tileH 瓦片高度
     * @return 生成的瓦片元数据列表
     */
    std::vector<TileMeta> split(
        const std::string& inputPath, 
        const std::string& outDir,
        int tileW, 
        int tileH
    );

private:
    /**
     * @brief 构建四叉树
     * 
     * @param imageData 图像数据指针
     * @param imageWidth 图像宽度
     * @param imageHeight 图像高度
     * @param config 分割配置
     * @return 四叉树根节点
     */
    std::unique_ptr<QuadTreeNode> buildQuadTree(
        const unsigned char* imageData, 
        int imageWidth, 
        int imageHeight,
        const Config& config
    );

    /**
     * @brief 递归分割节点
     * 
     * @param node 当前节点
     * @param imageData 图像数据指针
     * @param imageWidth 图像宽度
     * @param imageHeight 图像高度
     * @param config 分割配置
     * @param currentDepth 当前深度
     */
    void subdivideNode(
        QuadTreeNode* node, 
        const unsigned char* imageData, 
        int imageWidth, 
        int imageHeight,
        const Config& config,
        int currentDepth
    );

    /**
     * @brief 收集叶子节点并生成瓦片
     * 
     * @param node 当前节点
     * @param imageData 图像数据指针
     * @param imageWidth 图像宽度
     * @param imageHeight 图像高度
     * @param outDir 输出目录
     * @param tiles 瓦片元数据列表（输出）
     */
    void collectLeafTiles(
        const QuadTreeNode* node, 
        const unsigned char* imageData,
        int imageWidth,
        int imageHeight,
        const std::string& outDir,
        std::vector<TileMeta>& tiles
    );

    /**
     * @brief 生成单个瓦片文件（仅用于非纯色瓦片）
     * 
     * @param imageData 图像数据指针
     * @param imageWidth 图像宽度
     * @param imageHeight 图像高度
     * @param x 瓦片X坐标
     * @param y 瓦片Y坐标
     * @param width 瓦片宽度
     * @param height 瓦片高度
     * @param outputPath 输出文件路径
     * @return 是否成功生成
     */
    bool generateTile(
        const unsigned char* imageData,
        int imageWidth,
        int imageHeight,
        int x, int y, int width, int height,
        const std::string& outputPath
    );

    /**
     * @brief 确保输出目录存在
     * 
     * @param outDir 输出目录路径
     * @return 是否成功创建或已存在
     */
    bool ensureDirectoryExists(const std::string& outDir);

    /**
     * @brief 生成瓦片文件名
     * 
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @return 文件名
     */
    std::string generateTileFileName(int x, int y, int width, int height) const;

    /**
     * @brief 判断瓦片是否为纯色瓦片
     * 
     * 纯色瓦片的文件名为8位十六进制RGBA值（如 "FF0000FF"）
     * 
     * @param fileName 瓦片文件名
     * @return true 如果是纯色瓦片
     */
    static bool isPureColorTile(const std::string& fileName);

    /**
     * @brief 从纯色瓦片文件名解析RGBA颜色值
     * 
     * @param fileName 纯色瓦片文件名（8位十六进制）
     * @return RGBA颜色值
     */
    static uint32_t parseColorFromFileName(const std::string& fileName);

    ColorChecker colorChecker_;  ///< 颜色检查器实例
};

#endif // QUADTREESPLITTER_HPP
