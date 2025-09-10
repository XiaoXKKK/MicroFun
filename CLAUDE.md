# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MicroFun is a C++ research project focused on **client-side map loading optimization** for games. The core goal is to solve memory and loading speed issues with large game map resources through efficient map resource splitting and combination schemes. The project implements a quadtree-based algorithm to split large images into smaller tiles, with key optimizations like storing uniform color tiles as color values rather than image files.

## Build System & Commands

### Building the Project
The project uses CMake with presets for different build configurations:

```bash
# Using CMake presets (recommended)
cmake --preset=release    # Release build for production
cmake --preset=debug      # Debug build for development

# Build specific targets
cmake --build build/release --target split_tool
cmake --build build/release --target check_tool

# Manual CMake (if needed)
mkdir -p build/release && cd build/release
cmake ../.. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Running the Tools
The `run.sh` script provides a unified interface:

```bash
# Split a map image into tiles using quadtree algorithm
./run.sh split -i <input_image.png> -o <output_directory>

# Simulate viewport and assemble tiles (outputs hex pixel values by default)
./run.sh run -i <resource_dir> -p <x>,<y> -s <width>,<height>

# Generate PNG output (optional)
./run.sh run -i <resource_dir> -p <x>,<y> -s <width>,<height> -o output.png

# Legacy compatibility (check == run)
./run.sh check -i <resource_dir> -p <x>,<y> -s <width>,<height>
```

### Testing
```bash
# Run performance tests
./build/release/performance_test

# Run specific tool tests
./build/release/quadtree_split_test
./build/release/quadtree_index_test
```

## Core Architecture

### Key Components

The project is organized into three main parts:

1. **`lib/`** - Core C++ library with the following modules:
   - `TileSplitter`: Basic fixed-grid tile splitting using stb_image
   - `QuadTreeSplitter`: Advanced quadtree-based splitting with uniform color optimization
   - `TileIndex`/`QuadTreeIndex`: Spatial indexing for efficient viewport queries
   - `ViewportAssembler`/`EnhancedViewportAssembler`: Tile combination with alpha blending
   - `TileCache`: LRU caching system for loaded tiles
   - `AsyncTileLoader`: Asynchronous tile loading with thread pool

2. **`tools/`** - Command-line executables:
   - `split_main.cpp`: Image splitting tool
   - `check_main.cpp`: Viewport simulation tool
   - Test tools for quadtree functionality

3. **`data/`** - Sample data and output directory

### Data Flow

1. **Split Phase**: Large PNG → QuadTreeSplitter → Tiles + meta.txt index
2. **Runtime Phase**: Viewport query → QuadTreeIndex → Tile loading → ViewportAssembler → Final image

### Key Optimizations

- **Uniform Color Detection**: Pure color tiles stored as color values instead of image files
- **Quadtree Spatial Indexing**: Efficient viewport-to-tile mapping
- **Async Loading**: Non-blocking tile loading with configurable thread pool
- **LRU Caching**: Memory-efficient tile cache management
- **Alpha Blending**: Proper color composition for overlapping tiles

## Development Guidelines

### Code Organization
- Headers in `lib/include/`
- Implementation in `lib/src/`
- Tool entry points in `tools/`
- Use C++17 standard
- Follow existing naming conventions (PascalCase for classes, camelCase for functions)

### Dependencies
- **stb_image/stb_image_write**: Image I/O (included in project)
- **GoogleTest**: Unit testing framework (fetched by CMake)
- **Google Benchmark**: Performance testing (fetched by CMake)
- Standard C++17 libraries for filesystem, threading, etc.

### Performance Considerations
- Always call `stbi_image_free()` after loading images
- Use RAII for resource management
- Prefer move semantics for large data structures
- Consider memory budget when implementing caches
- Profile critical paths with the provided benchmark framework

### Platform Compatibility
- **Target Environment**: CentOS 7.9 x86_64, GCC 9.3.1/Clang 14.0.4
- Avoid platform-specific APIs
- Test with both debug and release builds
- Ensure compatibility with the specified hardware (Intel i7-11800H, 16GB RAM)

## Testing Strategy

### Unit Tests
Located in `tests/performance_test.cpp` with Google Test framework. Run via:
```bash
cd build/release && ctest
```

### Integration Tests
Tool-specific tests in `tools/quadtree_*_test.cpp` validate complete workflows.

### Performance Benchmarks
Use Google Benchmark for performance regression testing. Key metrics:
- Tile splitting time
- Viewport assembly latency
- Memory usage efficiency
- Cache hit rates

## Common Development Tasks

### Adding New Splitting Algorithm
1. Create header in `lib/include/NewSplitter.hpp`
2. Implement in `lib/src/NewSplitter.cpp`
3. Update `lib/CMakeLists.txt` to include new files
4. Add command-line option in `tools/split_main.cpp`
5. Add tests in `tests/` directory

### Optimizing Viewport Assembly
1. Profile current implementation with benchmark tools
2. Focus on hot paths in `ViewportAssembler` or `QuadTreeIndex`
3. Consider SIMD optimizations for pixel operations
4. Test memory allocation patterns

### Adding New Output Format
1. Extend `ViewportAssembler` to support new format
2. Add format detection in `tools/check_main.cpp`
3. Update `run.sh` help text and examples

## File Structure Conventions

```
MicroFun/
├── run.sh              # Main entry point
├── CMakeLists.txt      # Root build configuration
├── CMakePresets.json   # Build presets
├── lib/                # Core library
│   ├── CMakeLists.txt
│   ├── include/        # All header files
│   └── src/            # All implementation files
├── tools/              # Command-line tools
├── tests/              # Testing framework
├── data/               # Input/output data
├── bin/                # Built executables (generated)
└── build/              # Build artifacts (generated)
```

The `run.sh` script automatically handles building and copying executables to `bin/` directory.