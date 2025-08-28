#include "ColorChecker.hpp"

#include <cmath>

bool ColorChecker::isUniformColor(const unsigned char* imageData,
                                  int imageWidth, int x, int y, int width,
                                  int height, uint32_t& color) const {
    if (!imageData || width <= 0 || height <= 0) {
        return false;
    }

    // 获取第一个像素的颜色作为参考
    color = getPixelColor(imageData, imageWidth, x, y);

    // 检查区域内所有像素是否与参考颜色相同
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            uint32_t pixelColor =
                getPixelColor(imageData, imageWidth, x + dx, y + dy);
            if (!colorsEqual(pixelColor, color)) {
                return false;
            }
        }
    }

    return true;
}

bool ColorChecker::isUniformColor(const unsigned char* imageData,
                                  int imageWidth, int x, int y, int width,
                                  int height) const {
    uint32_t dummyColor;
    return isUniformColor(imageData, imageWidth, x, y, width, height,
                          dummyColor);
}

uint32_t ColorChecker::getPixelColor(const unsigned char* imageData,
                                     int imageWidth, int x, int y) const {
    // 计算像素在数据数组中的位置（RGBA格式，每像素4字节）
    const unsigned char* pixel = &imageData[(y * imageWidth + x) * 4];

    // 组合RGBA值为32位整数，格式为0xRRGGBBAA
    return (static_cast<uint32_t>(pixel[0]) << 24) |  // R
           (static_cast<uint32_t>(pixel[1]) << 16) |  // G
           (static_cast<uint32_t>(pixel[2]) << 8) |   // B
           static_cast<uint32_t>(pixel[3]);           // A
}

bool ColorChecker::colorsEqual(uint32_t color1, uint32_t color2) const {
    if (colorTolerance_ == 0) {
        // 严格相等
        return color1 == color2;
    }

    // 提取RGBA分量
    int r1 = (color1 >> 24) & 0xFF;
    int g1 = (color1 >> 16) & 0xFF;
    int b1 = (color1 >> 8) & 0xFF;
    int a1 = color1 & 0xFF;

    int r2 = (color2 >> 24) & 0xFF;
    int g2 = (color2 >> 16) & 0xFF;
    int b2 = (color2 >> 8) & 0xFF;
    int a2 = color2 & 0xFF;

    // 检查每个分量是否在容差范围内
    return std::abs(r1 - r2) <= colorTolerance_ &&
           std::abs(g1 - g2) <= colorTolerance_ &&
           std::abs(b1 - b2) <= colorTolerance_ &&
           std::abs(a1 - a2) <= colorTolerance_;
}

bool ColorChecker::isValidCoordinate(int imageWidth, int imageHeight, int x,
                                     int y) const {
    return x >= 0 && y >= 0 && x < imageWidth && y < imageHeight;
}
