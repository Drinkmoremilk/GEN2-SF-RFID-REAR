#!/bin/bash

set -e

# 获取当前脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GR_RFID_DIR="$SCRIPT_DIR/gr-rfid"
BUILD_DIR="$GR_RFID_DIR/build"
APP_DIR="$GR_RFID_DIR/apps"

echo "========================================"
echo "gr-rfid rebuild and run script"
echo "SCRIPT_DIR: $SCRIPT_DIR"
echo "GR_RFID_DIR: $GR_RFID_DIR"
echo "BUILD_DIR: $BUILD_DIR"
echo "========================================"

# 检查 gr-rfid 目录是否存在
if [ ! -d "$GR_RFID_DIR" ]; then
    echo "Error: gr-rfid directory not found at $GR_RFID_DIR"
    exit 1
fi

# 1. 删除旧的 build 文件夹
if [ -d "$BUILD_DIR" ]; then
    echo "Removing old build directory..."
    rm -rf "$BUILD_DIR"
fi

# 2. 新建 build 文件夹
echo "Creating new build directory..."
mkdir -p "$BUILD_DIR"

# 3. 进入 build 并重新 cmake
cd "$BUILD_DIR"
echo "Running cmake..."
cmake ..

# 4. 编译
echo "Building gr-rfid..."
make -j"$(nproc)"

# 如果你还需要安装到系统目录，可以取消下面这行注释
# sudo make install
# sudo ldconfig

# 5. 设置运行环境
export PYTHONPATH="$PYTHONPATH:$BUILD_DIR/python"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BUILD_DIR/lib"

echo "Environment configured:"
echo "PYTHONPATH=$PYTHONPATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

# 6. 运行 reader.py
echo "Starting Reader..."
cd "$APP_DIR"
sudo LD_LIBRARY_PATH="$LD_LIBRARY_PATH" \
     PYTHONPATH="$PYTHONPATH" \
     GR_SCHEDULER=TPB \
     nice -n -20 \
     python3 -u reader.py