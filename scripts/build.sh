#!/bin/bash

BUILD_TYPE=${1:-Release}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "Building project with type: $BUILD_TYPE"

# 清理旧的构建目录
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# 编译
make -j2

echo "Build completed successfully!"
