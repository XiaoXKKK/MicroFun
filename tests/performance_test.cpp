#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <string>

#include <iostream>

#include "QuadTreeIndex.hpp"
#include "TileIndex.hpp"
#include "ViewportAssembler.hpp"

// Fixture for benchmark tests
class ViewportBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        if (!tileIndex.load(const_resourceDir + "/meta.txt")) {
            // Fail the test if meta.txt is not found
            GTEST_FAIL() << "Failed to load meta.txt from " << const_resourceDir;
        }
        if (!quadTreeIndex.load(quad_resourceDir + "/meta.txt")) {
            GTEST_FAIL() << "Failed to load meta.txt from " << quad_resourceDir;
        }

        // Add this block to print statistics
        // if (state.thread_index() == 0) { // Only print once for multi-threaded benchmarks
        //     QuadTreeIndex::Statistics stats = quadTreeIndex.getStatistics();
        //     std::cout << "\n--- QuadTree Statistics ---" << std::endl;
        //     std::cout << "Total Nodes: " << stats.totalNodes << std::endl;
        //     std::cout << "Leaf Nodes: " << stats.leafNodes << std::endl;
        //     std::cout << "Max Depth: " << stats.maxDepth << std::endl;
        //     std::cout << "Total Tiles (in leaves): " << stats.totalTiles << std::endl;
        //     std::cout << "Avg Tiles Per Leaf: " << stats.avgTilesPerLeaf << std::endl;
        //     std::cout << "Actual tiles count: " << tileIndex.query({0, 0, 10000, 10000}).size() << std::endl;
        //     std::cout << "---------------------------\n" << std::endl;
        // }
    }

protected:
    const std::string const_resourceDir = "data/tiles";
    const std::string quad_resourceDir = "data/quad_tiles";
    TileIndex tileIndex;
    QuadTreeIndex quadTreeIndex;
};

// Benchmark for TileIndex::query (linear scan)
BENCHMARK_F(ViewportBenchmark, TileIndexQuery)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            Viewport vp = {i * 10, i * 5, 800, 600};
            auto tiles = tileIndex.query(vp);
            benchmark::DoNotOptimize(tiles);
        }
    }
}

// Benchmark for QuadTreeIndex::query (quadtree search)
BENCHMARK_F(ViewportBenchmark, QuadTreeIndexQuery)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            Viewport vp = {i * 10, i * 5, 800, 600};
            auto tiles = quadTreeIndex.query(vp);
            benchmark::DoNotOptimize(tiles);
        }
    }
}

// Main function to run benchmarks
BENCHMARK_MAIN();

/*
-------------------------------------------------------------------------------
Benchmark                                     Time             CPU   Iterations
-------------------------------------------------------------------------------
ViewportBenchmark/TileIndexQuery       55870991 ns     55804571 ns           14
ViewportBenchmark/QuadTreeIndexQuery   20026663 ns     20024657 ns           35

*/