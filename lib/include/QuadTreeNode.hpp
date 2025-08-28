#ifndef QUADTREENODE_HPP
#define QUADTREENODE_HPP

#include <cstdint>
#include <memory>
#include <vector>

/**
 * @brief 四叉树节点类，用于区域四叉树地图分割
 *
 * 每个节点代表图像中的一个正方形区域，可以递归分割为四个子区域。
 * 节点分为叶子节点（包含实际数据）和内部节点（包含子节点）。
 */
class QuadTreeNode {
 public:
  /**
   * @brief 构造一个四叉树节点
   * @param x 区域左上角X坐标
   * @param y 区域左上角Y坐标
   * @param width 区域宽度
   * @param height 区域高度
   */
  QuadTreeNode(int x, int y, int width, int height);

  /**
   * @brief 析构函数
   */
  ~QuadTreeNode() = default;

  /**
   * @brief 将当前节点分割为四个子节点
   *
   * 只有叶子节点且尺寸大于最小值才能分割。
   * 分割后生成四个子节点：左上、右上、左下、右下。
   */
  void subdivide();

  /**
   * @brief 检查是否为叶子节点
   * @return true 如果是叶子节点
   */
  bool isLeaf() const { return isLeaf_; }

  /**
   * @brief 获取区域X坐标
   * @return X坐标
   */
  int getX() const { return x_; }

  /**
   * @brief 获取区域Y坐标
   * @return Y坐标
   */
  int getY() const { return y_; }

  /**
   * @brief 获取区域尺寸
   * @return 区域宽度
   */
  int getWidth() const { return width_; }

  /**
   * @brief 获取区域高度
   * @return 区域高度
   */
  int getHeight() const { return height_; }

  /**
   * @brief 获取区域尺寸（兼容旧接口）
   * @return 区域宽度（为了向后兼容）
   */
  int getSize() const { return width_; }

  /**
   * @brief 获取统一颜色
   * @return RGBA颜色值，仅当区域颜色一致时有效
   */
  uint32_t getUniformColor() const { return uniformColor_; }

  /**
   * @brief 设置统一颜色
   * @param color RGBA颜色值
   */
  void setUniformColor(uint32_t color) { uniformColor_ = color; }

  /**
   * @brief 检查区域是否具有统一颜色
   * @return true 如果区域所有像素颜色相同
   */
  bool hasUniformColor() const { return hasUniformColor_; }

  /**
   * @brief 设置统一颜色标志
   * @param uniform 是否具有统一颜色
   */
  void setHasUniformColor(bool uniform) { hasUniformColor_ = uniform; }

  /**
   * @brief 获取子节点列表
   * @return 子节点向量的常引用
   */
  const std::vector<std::unique_ptr<QuadTreeNode>>& getChildren() const {
    return children_;
  }

  /**
   * @brief 获取子节点数量
   * @return 子节点数量
   */
  size_t getChildCount() const { return children_.size(); }

 private:
  int x_;                  ///< 区域左上角X坐标
  int y_;                  ///< 区域左上角Y坐标
  int width_;              ///< 区域宽度
  int height_;             ///< 区域高度
  bool isLeaf_;            ///< 是否为叶子节点
  uint32_t uniformColor_;  ///< 统一颜色（RGBA格式）
  bool hasUniformColor_;   ///< 是否具有统一颜色
  std::vector<std::unique_ptr<QuadTreeNode>>
      children_;  ///< 子节点列表（非叶子节点有4个子节点）
};

#endif  // QUADTREENODE_HPP
