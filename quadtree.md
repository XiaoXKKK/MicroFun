# 区域四叉树地图分割实施方案

## 项目概述

本文档描述了在 MicroFun 地图加载优化项目中实现区域四叉树分割方法的实施方案。该方案通过四叉树递归分割，根据区域颜色一致性来优化瓦片大小，以提高存储效率和加载性能。

## 1. 核心概念分析

### 当前实现分析
- **分割方式**：固定尺寸网格分割（如32x32瓦片）
- **特点**：简单直接，但无法根据内容优化
- **问题**：单色区域浪费存储，无法利用图像特征

### 目标实现
- **分割方式**：基于颜色一致性的四叉树分割
- **特点**：同色区域使用大瓦片，异色区域细分
- **优势**：存储效率高，保持视觉质量

## 2. 设计目标

### 主要目标
- **颜色感知分割**：相同颜色区域使用大瓦片，不同颜色区域细分
- **向后兼容**：保持现有的 TileMeta 结构和接口
- **简单高效**：算法简单，易于实现和调试

### 技术指标
- **存储效率**：对于有大面积单色区域的图像，显著减少瓦片数量
- **视觉质量**：完全保持原图质量（无损分割）
- **性能**：快速的颜色比较算法

## 3. 详细实现步骤

### 第一阶段：四叉树节点结构设计

#### 3.1 创建 QuadTreeNode 类
```cpp
struct QuadTreeNode {
    int x, y, size;                                      // 区域坐标和尺寸（正方形）
    bool isLeaf;                                         // 是否为叶子节点
    uint32_t uniformColor;                               // 统一颜色（如果区域颜色一致）
    std::vector<std::unique_ptr<QuadTreeNode>> children; // 子节点（非叶子节点）
    
    QuadTreeNode(int x, int y, int size) 
        : x(x), y(y), size(size), isLeaf(true), uniformColor(0) {}
    
    // 四个子节点（西北、东北、西南、东南）
    void subdivide() {
        if (isLeaf && size > 1) {
            int half = size / 2;
            children.resize(4);
            children[0] = std::make_unique<QuadTreeNode>(x, y, half);             // 西北
            children[1] = std::make_unique<QuadTreeNode>(x + half, y, half);      // 东北
            children[2] = std::make_unique<QuadTreeNode>(x, y + half, half);      // 西南
            children[3] = std::make_unique<QuadTreeNode>(x + half, y + half, half); // 东南
            isLeaf = false;
        }
    }
};
```

#### 3.2 颜色一致性检查
```cpp
class ColorChecker {
public:
    // 检查区域是否为统一颜色
    bool isUniformColor(const unsigned char* imageData, int imageWidth, 
                       int x, int y, int size, uint32_t& color);
    
    // 获取像素颜色（RGBA）
    uint32_t getPixelColor(const unsigned char* imageData, int imageWidth, int x, int y);
    
private:
    // 颜色比较容差（处理轻微的颜色差异）
    static constexpr int COLOR_TOLERANCE = 0;  // 严格相等
};
```

### 第二阶段：四叉树分割算法

#### 3.3 递归分割流程
```
1. 从根节点（整个图像）开始
2. 检查当前区域颜色一致性
3. 判断分割条件：
   - 如果颜色完全一致 → 标记为叶子节点，存储统一颜色
   - 如果达到最大深度 → 强制标记为叶子节点
   - 否则继续分割
4. 四等分当前区域
5. 递归处理四个子区域
```

#### 3.4 实现 QuadTreeSplitter 类
```cpp
class QuadTreeSplitter : public TileSplitter {
public:
    struct Config {
        int maxDepth = 8;                    // 最大分割深度
        int minTileSize = 4;                 // 最小瓦片尺寸
    };
    
    // 四叉树分割主接口
    std::vector<TileMeta> splitQuadTree(
        const std::string& inputPath, 
        const std::string& outDir,
        const Config& config = Config{}
    );
    
    // 兼容现有接口
    std::vector<TileMeta> split(
        const std::string& inputPath, 
        const std::string& outDir,
        int tileW, 
        int tileH
    ) override;

private:
    // 构建四叉树
    std::unique_ptr<QuadTreeNode> buildQuadTree(
        const unsigned char* imageData, 
        int imageWidth, 
        int imageHeight,
        const Config& config
    );
    
    // 递归分割节点
    void subdivideNode(
        QuadTreeNode* node, 
        const unsigned char* imageData, 
        int imageWidth, 
        int imageHeight,
        const Config& config,
        int currentDepth
    );
    
    // 收集叶子节点并生成瓦片
    void collectLeafTiles(
        const QuadTreeNode* node, 
        const unsigned char* imageData,
        int imageWidth,
        const std::string& outDir,
        std::vector<TileMeta>& tiles
    );
    
    ColorChecker colorChecker_;
};
```

### 第三阶段：瓦片生成与优化

#### 3.5 叶子节点处理
- **统一颜色区域**：生成单色瓦片或使用颜色填充
- **混合颜色区域**：直接从原图提取对应区域
- **瓦片命名**：使用区域坐标和尺寸命名（如 `qtile_0_0_64.png`）

#### 3.6 内存优化
- **按需生成**：只在需要时生成实际瓦片文件
- **颜色压缩**：统一颜色区域可以用更小的文件表示
- **索引优化**：在 meta.txt 中记录是否为统一颜色区域

## 4. 核心算法实现

### 4.1 颜色一致性检查算法
```cpp
bool ColorChecker::isUniformColor(const unsigned char* imageData, int imageWidth, 
                                 int x, int y, int size, uint32_t& color) {
    if (size <= 0) return false;
    
    // 获取第一个像素的颜色作为参考
    color = getPixelColor(imageData, imageWidth, x, y);
    
    // 检查区域内所有像素是否与参考颜色相同
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            uint32_t pixelColor = getPixelColor(imageData, imageWidth, x + dx, y + dy);
            if (pixelColor != color) {
                return false;
            }
        }
    }
    return true;
}

uint32_t ColorChecker::getPixelColor(const unsigned char* imageData, int imageWidth, int x, int y) {
    const unsigned char* pixel = &imageData[(y * imageWidth + x) * 4];
    return (pixel[0] << 24) | (pixel[1] << 16) | (pixel[2] << 8) | pixel[3];  // RGBA
}
```

### 4.2 递归分割算法
```cpp
void QuadTreeSplitter::subdivideNode(QuadTreeNode* node, 
                                    const unsigned char* imageData, 
                                    int imageWidth, int imageHeight,
                                    const Config& config, int currentDepth) {
    // 检查终止条件
    uint32_t uniformColor;
    bool isUniform = colorChecker_.isUniformColor(imageData, imageWidth, 
                                                 node->x, node->y, node->size, uniformColor);
    
    // 终止条件：1. 颜色一致 2. 达到最大深度 3. 达到最小尺寸
    if (isUniform || currentDepth >= config.maxDepth || node->size <= config.minTileSize) {
        node->isLeaf = true;
        if (isUniform) {
            node->uniformColor = uniformColor;
        }
        return;
    }
    
    // 四等分并递归处理
    node->subdivide();
    for (auto& child : node->children) {
        subdivideNode(child.get(), imageData, imageWidth, imageHeight, config, currentDepth + 1);
    }
}
```

## 5. 集成与接口扩展

### 5.1 扩展命令行接口
```bash
# 四叉树分割模式
./run.sh split -i input.png -o tiles/ --quadtree --max-depth 8 --min-size 4

# 保持固定尺寸模式兼容性
./run.sh split -i input.png -o tiles/ --tile 32x32

# 对比模式（同时生成两种分割结果）
./run.sh split -i input.png -o tiles/ --compare
```

### 5.2 更新 split_main.cpp
```cpp
int main(int argc, char** argv) {
    std::string input;
    std::string outDir = "data/tiles";
    bool useQuadTree = false;
    QuadTreeSplitter::Config quadTreeConfig;
    int tileW = 32, tileH = 32;  // 固定模式参数
    
    // 命令行参数解析
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if(arg == "--quadtree") {
            useQuadTree = true;
        }
        else if(arg == "--max-depth" && i+1 < argc) {
            quadTreeConfig.maxDepth = std::stoi(argv[++i]);
        }
        else if(arg == "--min-size" && i+1 < argc) {
            quadTreeConfig.minTileSize = std::stoi(argv[++i]);
        }
        // ... 其他参数处理
    }
    
    // 选择分割算法
    std::vector<TileMeta> tiles;
    if(useQuadTree) {
        QuadTreeSplitter splitter;
        tiles = splitter.splitQuadTree(input, outDir, quadTreeConfig);
    } else {
        TileSplitter splitter;
        tiles = splitter.split(input, outDir, tileW, tileH);
    }
    
    // 保存索引
    TileIndex index;
    index.setTiles(tiles);
    std::string metaFile = outDir + "/meta.txt";
    if(!index.save(metaFile)) {
        std::cerr << "Failed to save meta file\n";
        return 2;
    }
    
    std::cout << "Split completed: " << tiles.size() << " tiles generated\n";
    return 0;
}
```

## 6. 文件组织结构

```
lib/include/
├── QuadTreeSplitter.hpp        # 四叉树分割器
├── QuadTreeNode.hpp            # 四叉树节点定义
└── ColorChecker.hpp            # 颜色一致性检查

lib/src/
├── QuadTreeSplitter.cpp
├── QuadTreeNode.cpp
└── ColorChecker.cpp

tools/
└── split_main.cpp              # 修改：添加四叉树模式支持
```

## 7. 关键技术决策

### 7.1 参数配置
- **最小瓦片尺寸**：4x4 像素（平衡精度和性能）
- **最大分割深度**：8 层（控制计算复杂度和瓦片数量）
- **颜色容差**：0（严格颜色相等，确保无损分割）

### 7.2 算法选择
- **颜色比较**：RGBA 精确匹配（无损分割）
- **分割策略**：深度优先递归分割
- **存储格式**：保持与现有系统兼容的 PNG + meta.txt

## 8. 测试与验证

### 8.1 功能测试
```cpp
class QuadTreeSplitterTest {
public:
    void testUniformColorDetection();    // 统一颜色检测
    void testQuadTreeSplitting();       // 四叉树分割功能
    void testBoundaryConditions();      // 边界条件处理
    void testDepthLimiting();           // 深度限制验证
};
```

### 8.2 性能基准
- **测试图像**：包含大面积单色区域的地图图像
- **对比指标**：瓦片数量、文件大小、处理时间
- **质量验证**：确保重组后图像与原图完全一致

## 9. 预期效果

### 9.1 存储效率
- **单色区域优化**：大面积单色区域显著减少瓦片数量
- **保持质量**：非单色区域保持原有精度
- **文件大小**：单色瓦片可以进一步压缩

### 9.2 实施简单性
- **算法简单**：易于理解和实现
- **调试友好**：分割逻辑清晰，便于问题定位
- **渐进实施**：可以逐步替换现有分割方法

## 10. 总结

这个简化的四叉树分割方案专注于基础的颜色一致性分割，避免了复杂的自适应算法。通过递归检查区域颜色一致性，在保证完全无损的前提下，对包含大面积单色区域的地图实现存储优化。

该方案的核心优势：
- **简单可靠**：基于颜色完全匹配的简单算法
- **无损分割**：保证重组后图像与原图完全一致
- **易于实现**：核心算法逻辑清晰，开发风险低
- **向后兼容**：完全保持现有接口和文件格式
- **渐进优化**：为未来更复杂的算法奠定基础
