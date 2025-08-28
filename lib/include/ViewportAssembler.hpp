#pragma once
#include <string>
#include <vector>

#include "TileIndex.hpp"

class ViewportAssembler {
   public:
    bool assemble(const TileIndex& index, const Viewport& vp,
                  const std::string& resourceDir,
                  const std::string& outFile) const;
    std::string assembleToHex(const TileIndex& index, const Viewport& vp,
                              const std::string& resourceDir) const;

   private:
    void blit(std::vector<unsigned char>& canvas, int canvas_w, int canvas_h,
              const unsigned char* src, int sw, int sh, int stride, int dstX,
              int dstY) const;
    
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
    
    /**
     * @brief 为纯色瓦片填充canvas
     * 
     * @param canvas 画布数据
     * @param canvas_w 画布宽度
     * @param canvas_h 画布高度
     * @param color RGBA颜色值
     * @param w 瓦片宽度
     * @param h 瓦片高度
     * @param dstX 目标X坐标
     * @param dstY 目标Y坐标
     */
    void blitSolidColor(std::vector<unsigned char>& canvas, int canvas_w,
                        int canvas_h, uint32_t color, int w, int h, int dstX,
                        int dstY) const;
};
