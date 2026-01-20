#!/bin/bash
# Full build script - builds both Rust WASM and Qt WASM
set -e

echo "========================================"
echo "Airgap JSON Formatter - Full Build"
echo "========================================"
echo ""

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"

# Create dist directory
mkdir -p dist

# Step 1: Build Rust WASM
echo "[1/3] Building Rust WASM..."
"$SCRIPT_DIR/build-rust.sh"
echo ""

# Step 2: Copy static assets
echo "[2/3] Copying static assets..."
cp -r public/* dist/
echo "  Copied public/ -> dist/"
echo ""

# Step 3: Build Qt WASM (optional - skip if Qt not available)
echo "[3/3] Building Qt WASM..."
if [ -n "$QT_WASM_PATH" ] || [ -n "$SKIP_QT_BUILD" ]; then
    if [ -n "$SKIP_QT_BUILD" ]; then
        echo "  Skipping Qt build (SKIP_QT_BUILD is set)"
    else
        "$SCRIPT_DIR/build-qt.sh" || {
            echo "  Warning: Qt WASM build failed. Continuing without Qt..."
        }
    fi
else
    echo "  Skipping Qt build (QT_WASM_PATH not set)"
    echo "  Set QT_WASM_PATH to enable Qt WASM build"
fi
echo ""

# Summary
echo "========================================"
echo "Build Complete!"
echo "========================================"
echo ""
echo "Output directory: dist/"
echo ""
echo "Contents:"
ls -la dist/
echo ""
echo "To serve locally:"
echo "  python3 -m http.server 8080 --directory dist"
echo ""
echo "Then open: http://localhost:8080"
