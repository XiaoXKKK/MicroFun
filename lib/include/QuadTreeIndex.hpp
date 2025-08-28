#pragma once
#include "TileIndex.hpp"
#include "QuadTreeNode.hpp"
#include <memory>
#include <unordered_set>

/**
 * @brief 扩展的四叉树节点，用于空间索引
 * 基于现有的QuadTreeNode，添加瓦片索引功能
 */
struct IndexQuadTreeNode {
    std::unique_ptr<QuadTreeNode> node;                  // 原始四叉树节点
    std::vector<int> tileIndices;                        // 存储在此节点的瓦片索引
    std::vector<std::unique_ptr<IndexQuadTreeNode>> children; // 子节点
    
    IndexQuadTreeNode(int x, int y, int width, int height) 
        : node(std::make_unique<QuadTreeNode>(x, y, width, height)) {}
    
    /**
     * @brief 检查矩形是否与节点相交
     */
    bool intersects(int rx, int ry, int rw, int rh) const {
        int nx = node->getX();
        int ny = node->getY(); 
        int nw = node->getWidth();
        int nh = node->getHeight();
        return !(rx >= nx + nw || ry >= ny + nh || rx + rw <= nx || ry + rh <= ny);
    }
    
    /**
     * @brief 四等分节点
     */
    void subdivide() {
        if (node->isLeaf() && (node->getWidth() > 1 || node->getHeight() > 1)) {
            node->subdivide(); // 分割原始节点
            
            // 创建对应的索引子节点
            children.resize(4);
            const auto& origChildren = node->getChildren();
            for (int i = 0; i < 4; ++i) {
                const auto& child = origChildren[i];
                children[i] = std::make_unique<IndexQuadTreeNode>(
                    child->getX(), child->getY(), 
                    child->getWidth(), child->getHeight()
                );
            }
        }
    }
};

/**
 * @brief 基于四叉树的瓦片索引类，用于优化空间查询
 */
class QuadTreeIndex : public TileIndex {
public:
    struct Config {
        int maxDepth;        // 最大分割深度
        int maxTilesPerNode; // 每个节点最大瓦片数量
        
        Config() : maxDepth(8), maxTilesPerNode(8) {}
    };
    
    /**
     * @brief 构造函数
     * @param config 四叉树配置参数
     */
    explicit QuadTreeIndex(const Config& config = Config());
    
    /**
     * @brief 从meta文件加载瓦片数据并构建四叉树
     * @param metaFile meta.txt文件路径
     * @return 是否加载成功
     */
    bool load(const std::string& metaFile);
    
    /**
     * @brief 查询与视口相交的瓦片（使用四叉树优化）
     * @param vp 视口范围
     * @return 相交的瓦片列表
     */
    std::vector<TileMeta> query(const Viewport& vp) const;
    
    /**
     * @brief 获取四叉树统计信息
     */
    struct Statistics {
        int totalNodes = 0;
        int leafNodes = 0;
        int maxDepth = 0;
        int totalTiles = 0;
        double avgTilesPerLeaf = 0.0;
    };
    
    Statistics getStatistics() const;
    
private:
    Config config_;
    std::unique_ptr<IndexQuadTreeNode> root_;
    
    /**
     * @brief 构建四叉树
     */
    void buildQuadTree();
    
    /**
     * @brief 递归插入瓦片到四叉树节点
     * @param node 当前节点
     * @param tileIndex 瓦片在tiles_数组中的索引
     * @param depth 当前深度
     */
    void insertTile(IndexQuadTreeNode* node, int tileIndex, int depth);
    
    /**
     * @brief 递归查询与视口相交的瓦片
     * @param node 当前节点
     * @param vp 视口
     * @param result 结果集
     * @param visited 已访问的瓦片索引集合（避免重复）
     */
    void queryRecursive(const IndexQuadTreeNode* node, const Viewport& vp, 
                       std::vector<TileMeta>& result, std::unordered_set<int>& visited) const;
    
    /**
     * @brief 计算统计信息（递归）
     */
    void calculateStatistics(const IndexQuadTreeNode* node, Statistics& stats, int depth) const;
};
