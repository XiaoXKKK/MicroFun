#include "QuadTreeNode.hpp"

QuadTreeNode::QuadTreeNode(int x, int y, int width, int height)
    : x_(x),
      y_(y),
      width_(width),
      height_(height),
      isLeaf_(true),
      uniformColor_(0),
      hasUniformColor_(false) {
    // 构造函数初始化所有成员变量
    // 默认创建为叶子节点，没有统一颜色
}

void QuadTreeNode::subdivide() {
    // 只有叶子节点且尺寸大于最小值才能分割
    if (!isLeaf_ || width_ <= 1 || height_ <= 1) {
        return;
    }

    // 计算子节点的尺寸
    int halfWidth = width_ / 2;
    int halfHeight = height_ / 2;

    // 如果尺寸太小，不进行分割
    if (halfWidth <= 0 || halfHeight <= 0) {
        return;
    }

    // 预分配4个子节点的空间
    children_.reserve(4);

    // 创建四个子节点：左上、右上、左下、右下
    children_.push_back(
        std::make_unique<QuadTreeNode>(x_, y_, halfWidth, halfHeight));  // 左上
    children_.push_back(std::make_unique<QuadTreeNode>(
        x_ + halfWidth, y_, width_ - halfWidth, halfHeight));  // 右上
    children_.push_back(std::make_unique<QuadTreeNode>(
        x_, y_ + halfHeight, halfWidth, height_ - halfHeight));  // 左下
    children_.push_back(std::make_unique<QuadTreeNode>(
        x_ + halfWidth, y_ + halfHeight, width_ - halfWidth,
        height_ - halfHeight));  // 右下

    // 标记为非叶子节点
    isLeaf_ = false;
}
