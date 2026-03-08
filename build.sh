#!/bin/bash
# =============================================================================
# Mood_1.0 Fast Build Script
# 
# Optimized for Docker on Windows:
#   - Copies sources INTO container's fast ext4 filesystem
#   - Builds with parallel jobs (-j)
#   - Only copies artifacts BACK to host
#   - Avoids slow Windows volume mount I/O during compilation
#
# Usage: ./build.sh [clean|debug|release|help]
# =============================================================================
set -e

NPROCS=$(nproc 2>/dev/null || echo 4)
BUILD_WS="/tmp/mood_build"

echo "========================================"
echo " Mood_1.0 Fast Build (${NPROCS} cores)"
echo "========================================"

# Parse arguments
MAKE_TARGET="all"
EXTRA_FLAGS=""
DO_CLEAN=0

case "${1:-}" in
    clean)   DO_CLEAN=1 ;;
    debug)   EXTRA_FLAGS="OPT='-Og -g3'" ;;
    release) EXTRA_FLAGS="OPT=-Os" ;;
    help)
        echo "Usage: $0 [clean|debug|release|help]"
        echo "  (default)  Build project with -j${NPROCS}"
        echo "  clean      Clean and rebuild"
        echo "  debug      Debug build (-Og -g3)"
        echo "  release    Release build (-Os)"
        exit 0
        ;;
esac

START_TIME=$(date +%s%N)

# 1. Copy sources to fast filesystem (skip Output, .git, large archives)
echo "[1/3] Copying sources to container filesystem..."
rm -rf "$BUILD_WS"
mkdir -p "$BUILD_WS"
cd /workspace
tar cf - \
    --exclude='Output' \
    --exclude='.git' \
    --exclude='*.bz2' \
    --exclude='*.tar' \
    --exclude='*.log' \
    . | (cd "$BUILD_WS" && tar xf -)

mkdir -p "$BUILD_WS/Output/build"

# Fix case-sensitivity: Windows source files use uppercase dir names
# that differ from actual lowercase directory names on Linux
cd "$BUILD_WS/Drivers/SYSTEM"
[ ! -e SYS ]   && ln -s sys SYS   || true
[ ! -e USART ] && ln -s usart USART || true
[ ! -e DELAY ] && ln -s delay DELAY || true

# 2. Build with parallel jobs
cd "$BUILD_WS"
if [ "$DO_CLEAN" = "1" ]; then
    echo "[2/3] Clean + Build (-j${NPROCS})..."
    make clean
    make -j${NPROCS} all $EXTRA_FLAGS
else
    echo "[2/3] Building (-j${NPROCS})..."
    make -j${NPROCS} all $EXTRA_FLAGS
fi

# 3. Copy artifacts back to host
echo "[3/3] Copying artifacts to host..."
mkdir -p /workspace/Output
for f in Mood_1.0.elf Mood_1.0.hex Mood_1.0.bin Mood_1.0.map; do
    [ -f "$BUILD_WS/Output/$f" ] && cp -f "$BUILD_WS/Output/$f" /workspace/Output/
done

# Summary
END_TIME=$(date +%s%N)
ELAPSED=$(( (END_TIME - START_TIME) / 1000000000 ))
echo ""
echo "========================================"
echo " Build Complete! (${ELAPSED}s)"
echo "========================================"
ls -lh /workspace/Output/Mood_1.0.{elf,hex,bin} 2>/dev/null | awk '{printf "  %-30s %s\n", $NF, $5}'
