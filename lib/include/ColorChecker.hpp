#ifndef COLORCHECKER_HPP
#define COLORCHECKER_HPP

#include <cstdint>

/**
 * @brief 颜色检查器类，用于检查图像区域的颜色一致性
 *
 * 提供检查指定区域是否具有统一颜色的功能，支持RGBA格式图像。
 */
class ColorChecker {
   public:
    /**
     * @brief 构造函数
     */
    ColorChecker() = default;

    /**
     * @brief 析构函数
     */
    ~ColorChecker() = default;

    /**
     * @brief 检查指定区域是否具有统一颜色
     *
     * @param imageData 图像数据指针（RGBA格式，4字节每像素）
     * @param imageWidth 图像宽度（像素）
     * @param x 检查区域左上角X坐标
     * @param y 检查区域左上角Y坐标
     * @param width 检查区域宽度
     * @param height 检查区域高度
     * @param color 输出参数，如果区域颜色一致则返回该颜色
     * @return true 如果区域内所有像素颜色相同
     */
    bool isUniformColor(const unsigned char* imageData, int imageWidth, int x,
                        int y, int width, int height, uint32_t& color) const;

    /**
     * @brief 获取指定位置像素的颜色
     *
     * @param imageData 图像数据指针（RGBA格式）
     * @param imageWidth 图像宽度（像素）
     * @param x 像素X坐标
     * @param y 像素Y坐标
     * @return RGBA颜色值，格式为0xRRGGBBAA
     */
    uint32_t getPixelColor(const unsigned char* imageData, int imageWidth,
                           int x, int y) const;

    /**
     * @brief 检查指定区域是否具有统一颜色（重载版本，不返回颜色值）
     *
     * @param imageData 图像数据指针（RGBA格式，4字节每像素）
     * @param imageWidth 图像宽度（像素）
     * @param x 检查区域左上角X坐标
     * @param y 检查区域左上角Y坐标
     * @param width 检查区域宽度
     * @param height 检查区域高度
     * @return true 如果区域内所有像素颜色相同
     */
    bool isUniformColor(const unsigned char* imageData, int imageWidth, int x,
                        int y, int width, int height) const;

    /**
     * @brief 比较两个颜色是否相等
     *
     * @param color1 第一个颜色值
     * @param color2 第二个颜色值
     * @return true 如果两个颜色相等（在容差范围内）
     */
    bool colorsEqual(uint32_t color1, uint32_t color2) const;

    /**
     * @brief 设置颜色比较容差
     *
     * @param tolerance 容差值（0-255）
     */
    void setColorTolerance(int tolerance) { colorTolerance_ = tolerance; }

    /**
     * @brief 获取当前颜色比较容差
     *
     * @return 容差值
     */
    int getColorTolerance() const { return colorTolerance_; }

   private:
    /**
     * @brief 颜色比较容差，用于处理轻微的颜色差异
     * 默认为0，表示严格相等
     */
    int colorTolerance_ = 0;

    /**
     * @brief 检查坐标是否在图像范围内
     *
     * @param imageWidth 图像宽度
     * @param imageHeight 图像高度（可以从其他参数推导）
     * @param x X坐标
     * @param y Y坐标
     * @return true 如果坐标在范围内
     */
    bool isValidCoordinate(int imageWidth, int imageHeight, int x, int y) const;
};

#endif  // COLORCHECKER_HPP
