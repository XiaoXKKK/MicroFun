#include "QuadTreeIndex.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

QuadTreeIndex::QuadTreeIndex(const Config& config)
    : config_(config), root_(nullptr) {}

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
    root_ = std::make_unique<IndexQuadTreeNode>(0, 0, getMapWidth(),
                                                getMapHeight());

    // 将所有瓦片插入四叉树
    for (int i = 0; i < static_cast<int>(tiles_.size()); ++i) {
        insertTile(root_.get(), i, 0);
    }
}

void QuadTreeIndex::insertTile(IndexQuadTreeNode* node, int tileIndex,
                               int depth) {
    const TileMeta& tile = tiles_[tileIndex];

    // 检查瓦片是否与节点相交
    if (!node->intersects(tile.x, tile.y, tile.w, tile.h)) {
        return;
    }

    // 如果是叶子节点
    if (node->node->isLeaf()) {
        // 检查瓦片是否完全包含在当前节点内
        if (node->contains(tile.x, tile.y, tile.w, tile.h)) {
            node->tileIndices.push_back(tileIndex);
        } else {
            // 瓦片跨越边界，放在当前节点
            node->tileIndices.push_back(tileIndex);
        }

        // 检查是否需要分割节点
        if (node->tileIndices.size() >
                static_cast<size_t>(config_.maxTilesPerNode) &&
            depth < config_.maxDepth &&
            (node->node->getWidth() > 1 || node->node->getHeight() > 1)) {
            // 分割节点
            node->subdivide();

            // 重新分配瓦片：只有完全包含在子节点内的瓦片才移动到子节点
            std::vector<int> remainingTiles;
            
            for (int oldTileIndex : node->tileIndices) {
                const TileMeta& oldTile = tiles_[oldTileIndex];
                bool movedToChild = false;
                
                for (auto& child : node->children) {
                    if (child->contains(oldTile.x, oldTile.y, oldTile.w, oldTile.h)) {
                        insertTile(child.get(), oldTileIndex, depth + 1);
                        movedToChild = true;
                        break;  // 瓦片只能完全属于一个子节点
                    }
                }
                
                // 如果瓦片跨越多个子节点，保留在当前节点
                if (!movedToChild) {
                    remainingTiles.push_back(oldTileIndex);
                }
            }
            
            node->tileIndices = std::move(remainingTiles);
        }
    } else {
        // 非叶子节点：只对完全包含的瓦片递归插入
        bool inserted = false;
        for (auto& child : node->children) {
            if (child->contains(tile.x, tile.y, tile.w, tile.h)) {
                insertTile(child.get(), tileIndex, depth + 1);
                inserted = true;
                break;  // 瓦片只能完全属于一个子节点
            }
        }
        
        // 如果瓦片不能完全包含在任何子节点中，存储在当前节点
        if (!inserted) {
            node->tileIndices.push_back(tileIndex);
        }
    }
}

std::vector<TileMeta> QuadTreeIndex::query(const Viewport& vp) const {
    std::vector<TileMeta> result;
    if (!root_) {
        return result;
    }

    queryRecursive(root_.get(), vp, result);
    return result;
}

void QuadTreeIndex::queryRecursive(const IndexQuadTreeNode* node,
                                   const Viewport& vp,
                                   std::vector<TileMeta>& result) const {
    // 检查节点是否与视口相交
    if (!node->intersects(vp.x, vp.y, vp.w, vp.h)) {
        return;
    }

    // 检查当前节点存储的瓦片
    for (int tileIndex : node->tileIndices) {
        const TileMeta& tile = tiles_[tileIndex];
        // 检查瓦片是否与视口相交
        bool overlap =
            !(tile.x + tile.w <= vp.x || tile.y + tile.h <= vp.y ||
              tile.x >= vp.x + vp.w || tile.y >= vp.y + vp.h);
        if (overlap) {
            result.push_back(tile);
        }
    }

    // 递归查询子节点
    if (!node->node->isLeaf()) {
        for (const auto& child : node->children) {
            queryRecursive(child.get(), vp, result);
        }
    }
}

QuadTreeIndex::Statistics QuadTreeIndex::getStatistics() const {
    Statistics stats;
    if (root_) {
        calculateStatistics(root_.get(), stats, 0);
        if (stats.leafNodes > 0) {
            stats.avgTilesPerLeaf =
                static_cast<double>(stats.totalTiles) / stats.leafNodes;
        }
    }
    return stats;
}

void QuadTreeIndex::calculateStatistics(const IndexQuadTreeNode* node,
                                        Statistics& stats, int depth) const {
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
