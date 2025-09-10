#!/usr/bin/env bash
set -euo pipefail

# Orchestrator for map split & runtime viewport assembly preview.
# 根据规范：
#   split  预处理拆分地图: ./run.sh split -i <input_map_path> -o <output_dir>
#   run    视口组合预览:   ./run.sh run   -i <resource_dir> -p <posx>,<posy> -s <w>,<h>
# 兼容旧命令 check (与 run 等价)。
# 构建产物位于 build/，工具复制到 bin/

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build/release"
BIN_DIR="$ROOT_DIR/bin"

ensure_build() {
  echo "[Build] Compiling tools (release)..."
  mkdir -p "$BUILD_DIR" "$BIN_DIR"
  (cd "$BUILD_DIR" && cmake ../.. -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build . --config Release >/dev/null)
  cp "$BUILD_DIR/split_tool" "$BIN_DIR/" || true
  cp "$BUILD_DIR/check_tool" "$BIN_DIR/" || true
}

cmd=${1:-}; shift || true
case "$cmd" in
  split)
    # ensure_build
    exec "$BIN_DIR/split_tool" "$@"
    ;;
  run|check)
    # ensure_build
    exec "$BIN_DIR/check_tool" "$@"
    ;;
  *)
    cat <<EOF
Map Loading Optimization Runner
Commands:
  split  -i <input_map.png> -o <output_dir>
  run    -i <resource_dir> -p <posx>,<posy> -s <w>,<h>
Notes:
  * run 与 check 等价（向后兼容）。
Output (run):
  连续输出 w*h 个像素的 0xRRGGBBAA 十六进制值，逗号分隔。
Examples:
  ./run.sh split -i data/sample1.png -o data/tiles
  ./run.sh run -i data/tiles -p 0,0 -s 128,128
EOF
    ;;
esac
