#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "QuadTreeIndex.hpp"
#include "TileIndex.hpp"
#include "ViewportAssembler.hpp"
#include "EnhancedViewportAssembler.hpp"
#include "TileCache.hpp"
#include "AsyncTileLoader.hpp"
#include "stb_image.h"

int main(int argc, char** argv) {
    std::string resourceDir = "data/tiles";
    int px = 0, py = 0, sw = 128, sh = 128;
    bool outputPNG = false;
    std::string outFile = "";
    bool useQuadTree = false;
    bool useEnhanced = false;
    bool enableCache = true;
    bool enableAsync = true;
    bool showStats = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-i" && i + 1 < argc)
            resourceDir = argv[++i];
        else if (a == "-p" && i + 1 < argc) {
            std::string v = argv[++i];
            auto c = v.find(',');
            if (c != std::string::npos) {
                px = std::stoi(v.substr(0, c));
                py = std::stoi(v.substr(c + 1));
            }
        } else if (a == "-s" && i + 1 < argc) {
            std::string v = argv[++i];
            auto x = v.find(',');
            if (x != std::string::npos) {
                sw = std::stoi(v.substr(0, x));
                sh = std::stoi(v.substr(x + 1));
            } else {
                x = v.find('x');
                if (x != std::string::npos) {
                    sw = std::stoi(v.substr(0, x));
                    sh = std::stoi(v.substr(x + 1));
                }
            }
        } else if (a == "-o" && i + 1 < argc) {
            outputPNG = true;
            outFile = argv[++i];
        } else if (a == "-q" || a == "--quadtree") {
            useQuadTree = true;
        } else if (a == "-e" || a == "--enhanced") {
            useEnhanced = true;
        } else if (a == "--no-cache") {
            enableCache = false;
        } else if (a == "--no-async") {
            enableAsync = false;
        } else if (a == "--stats") {
            showStats = true;
        } else if (a == "-h") {
            std::cout
                << "Usage: check_tool -i <resource_dir> -p posx,posy -s w,h "
                   "[-q|--quadtree] [-e|--enhanced] [--no-cache] [--no-async] [--stats] [-o <output.png>]\n"
                << "Options:\n"
                << "  -e, --enhanced    Use enhanced viewport assembler with caching and async loading\n"
                << "  --no-cache        Disable tile caching (only with --enhanced)\n"
                << "  --no-async        Disable async loading (only with --enhanced)\n"
                << "  --stats           Show cache and loader statistics\n";
            return 0;
        }
    }
    std::string meta = resourceDir + "/meta.txt";
    std::unique_ptr<TileIndex> index;
    if (useQuadTree) {
        index = std::make_unique<QuadTreeIndex>();
    } else {
        index = std::make_unique<TileIndex>();
    }
    if (!index->load(meta)) {
        std::cerr << "Failed load meta: " << meta << "\n";
        return 1;
    }
    
    int internalY = index->getMapHeight() - py - sh;
    if (internalY < 0) internalY = 0;
    
    Viewport vp{px, internalY, sw, sh};
    
    if (useEnhanced) {
        std::shared_ptr<TileCache> cache = nullptr;
        std::shared_ptr<AsyncTileLoader> loader = nullptr;
        
        if (enableCache) {
            TileCache::Config cacheConfig;
            cacheConfig.maxMemoryBytes = 256 * 1024 * 1024; // 256MB
            cacheConfig.maxTileCount = 5000;
            cache = std::make_shared<TileCache>(cacheConfig);
        }
        
        if (enableAsync) {
            AsyncTileLoader::Config loaderConfig;
            loaderConfig.numWorkerThreads = 2;
            loaderConfig.maxQueueSize = 500;
            loader = std::make_shared<AsyncTileLoader>(cache, loaderConfig);
            loader->start();
        }
        
        EnhancedViewportAssembler::Config assemblerConfig;
        assemblerConfig.enableAsyncLoading = enableAsync;
        assemblerConfig.enableCaching = enableCache;
        assemblerConfig.enablePreloading = true;
        
        EnhancedViewportAssembler assembler(cache, loader, assemblerConfig);
        
        if (outputPNG) {
            std::string png = outFile.empty() ? (resourceDir + "/viewport_enhanced.png") : outFile;
            if (!assembler.assemble(*index, vp, resourceDir, png)) {
                std::cerr << "Enhanced assemble failed\n";
                return 2;
            }
            std::cout << "Enhanced assemble OK -> " << png << "\n";
            
            if (showStats) {
                std::cout << "\n";
                assembler.printCacheStatistics();
                std::cout << "\n";
                assembler.printLoaderStatistics();
                
                auto stats = assembler.getLastAssemblyStats();
                std::cout << "\n=== Assembly Statistics ===\n";
                std::cout << "Total tiles: " << stats.totalTiles << "\n";
                std::cout << "Cached tiles: " << stats.cachedTiles << "\n";
                std::cout << "Async loaded: " << stats.asyncLoadedTiles << "\n";
                std::cout << "Sync loaded: " << stats.syncLoadedTiles << "\n";
                std::cout << "Failed tiles: " << stats.failedTiles << "\n";
                std::cout << "Cache hit rate: " << (stats.getCacheHitRate() * 100) << "%\n";
                std::cout << "Assembly time: " << stats.assemblyTimeMs << " ms\n";
            }
        } else {
            std::string hexResult = assembler.assembleToHex(*index, vp, resourceDir);
            if (hexResult.empty()) {
                return 1;
            }
            std::cout << hexResult << std::endl;
            
            if (showStats) {
                std::cerr << "\n";
                assembler.printCacheStatistics();
                std::cerr << "\n";
                assembler.printLoaderStatistics();
            }
        }
        
        if (loader) {
            loader->stop();
        }
        
        return 0;
    }
    
    // Fallback to original assembler
    ViewportAssembler assembler;
    if (outputPNG) {
        std::string png = outFile.empty() ? (resourceDir + "/viewport.png") : outFile;
        if (!assembler.assemble(*index, vp, resourceDir, png)) {
            std::cerr << "Assemble failed\n";
            return 2;
        }
        std::cout << "Assemble OK -> " << png << "\n";
        return 0;
    }

    std::string hexResult = assembler.assembleToHex(*index, vp, resourceDir);
    if (hexResult.empty()) {
        return 1;
    }
    std::cout << hexResult << std::endl;

    return 0;
}
