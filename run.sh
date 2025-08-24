#!/usr/bin/env bash
set -euo pipefail

# Simple orchestrator for map split & viewport check.
# Usage:
#   ./run.sh split -i input_map.png [-o out_dir] [--tile WxH]
#   ./run.sh check -i data/tiles/meta.txt -p X,Y -s WxH
# Build artifacts are placed under build/ and resulting tools copied to bin/

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
BIN_DIR="$ROOT_DIR/bin"

ensure_build() {
  if [[ ! -x "$BIN_DIR/split_tool" || ! -x "$BIN_DIR/check_tool" ]]; then
    echo "[Build] Compiling tools..."
    mkdir -p "$BUILD_DIR" "$BIN_DIR"
    (cd "$BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build . --config Release >/dev/null)
    cp "$BUILD_DIR/split_tool" "$BIN_DIR/" || true
    cp "$BUILD_DIR/check_tool" "$BIN_DIR/" || true
  fi
}

cmd=$1; shift || true
case "$cmd" in
  split)
    ensure_build
    # forward remaining args to split_tool
    exec "$BIN_DIR/split_tool" "$@"
    ;;
  check)
    ensure_build
    exec "$BIN_DIR/check_tool" "$@"
    ;;
  *)
    cat <<EOF
Map Loading Optimization Runner
Commands:
  split  -i <input_map.png> [-o out_dir] [--tile WxH] [--meta meta_file]
  check  -i <meta_file> -p X,Y -s WxH [-o out_png]
Examples:
  ./run.sh split -i data/world.png --tile 256x256
  ./run.sh check -i data/tiles/meta.txt -p 0,0 -s 512x512 -o data/viewport.png
EOF
    ;;
 esac
