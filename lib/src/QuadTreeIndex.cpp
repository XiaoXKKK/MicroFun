#include "QuadTreeIndex.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

QuadTreeIndex::QuadTreeIndex(const Config& config) : config_(config), root_(nullptr) {
}

bool QuadTreeIndex::load(const std::string& metaFile) {
    // 先使用基类方法加载瓦片数据
    if (!TileIndex::load(metaFile)) {
        return false;
    }
    
    // 构建四叉树
    buildQuadTree();
    return true;
}

void QuadTreeIndex::buildQuadTree() {
    if (tiles_.empty()) {
        return;
    }
    
    // 创建根节点，覆盖整个地图区域
    root_ = std::make_unique<IndexQuadTreeNode>(0, 0, getMapWidth(), getMapHeight());
    
    // 将所有瓦片插入四叉树
    for (int i = 0; i < static_cast<int>(tiles_.size()); ++i) {
        insertTile(root_.get(), i, 0);
    }
}

void QuadTreeIndex::insertTile(IndexQuadTreeNode* node, int tileIndex, int depth) {
    const TileMeta& tile = tiles_[tileIndex];
    
    // 检查瓦片是否与节点相交
    if (!node->intersects(tile.x, tile.y, tile.w, tile.h)) {
        return;
    }
    
    // 如果是叶子节点
    if (node->node->isLeaf()) {
        node->tileIndices.push_back(tileIndex);
        
        // 检查是否需要分割节点
        if (node->tileIndices.size() > static_cast<size_t>(config_.maxTilesPerNode) && 
            depth < config_.maxDepth && 
            (node->node->getWidth() > 1 || node->node->getHeight() > 1)) {
            
            // 分割节点
            node->subdivide();
            
            // 将当前节点的瓦片重新分配到子节点
            std::vector<int> tilesToRedistribute = std::move(node->tileIndices);
            node->tileIndices.clear();
            
            for (int oldTileIndex : tilesToRedistribute) {
                for (auto& child : node->children) {
                    insertTile(child.get(), oldTileIndex, depth + 1);
                }
            }
        }
    } else {
        // 如果不是叶子节点，递归插入到子节点
        for (auto& child : node->children) {
            insertTile(child.get(), tileIndex, depth + 1);
        }
    }
}

std::vector<TileMeta> QuadTreeIndex::query(const Viewport& vp) const {
    std::vector<TileMeta> result;
    if (!root_) {
        return result;
    }
    
    std::unordered_set<int> visited;
    queryRecursive(root_.get(), vp, result, visited);
    return result;
}

void QuadTreeIndex::queryRecursive(const IndexQuadTreeNode* node, const Viewport& vp, 
                                  std::vector<TileMeta>& result, std::unordered_set<int>& visited) const {
    // 检查节点是否与视口相交
    if (!node->intersects(vp.x, vp.y, vp.w, vp.h)) {
        return;
    }
    
    if (node->node->isLeaf()) {
        // 叶子节点：检查所有瓦片
        for (int tileIndex : node->tileIndices) {
            if (visited.find(tileIndex) != visited.end()) {
                continue; // 已经访问过
            }
            
            const TileMeta& tile = tiles_[tileIndex];
            // 检查瓦片是否与视口相交
            bool overlap = !(tile.x + tile.w <= vp.x || tile.y + tile.h <= vp.y || 
                           tile.x >= vp.x + vp.w || tile.y >= vp.y + vp.h);
            if (overlap) {
                result.push_back(tile);
                visited.insert(tileIndex);
            }
        }
    } else {
        // 非叶子节点：递归查询子节点
        for (const auto& child : node->children) {
            queryRecursive(child.get(), vp, result, visited);
        }
    }
}

QuadTreeIndex::Statistics QuadTreeIndex::getStatistics() const {
    Statistics stats;
    if (root_) {
        calculateStatistics(root_.get(), stats, 0);
        if (stats.leafNodes > 0) {
            stats.avgTilesPerLeaf = static_cast<double>(stats.totalTiles) / stats.leafNodes;
        }
    }
    return stats;
}

void QuadTreeIndex::calculateStatistics(const IndexQuadTreeNode* node, Statistics& stats, int depth) const {
    stats.totalNodes++;
    stats.maxDepth = std::max(stats.maxDepth, depth);
    
    if (node->node->isLeaf()) {
        stats.leafNodes++;
        stats.totalTiles += static_cast<int>(node->tileIndices.size());
    } else {
        for (const auto& child : node->children) {
            calculateStatistics(child.get(), stats, depth + 1);
        }
    }
}
