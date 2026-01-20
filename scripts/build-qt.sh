#!/bin/bash
# Build Qt WASM application
set -e

echo "Building Qt WASM application..."

# Check for Qt WASM environment
if [ -z "$QT_WASM_PATH" ]; then
    echo "Warning: QT_WASM_PATH not set. Attempting to find Qt WASM installation..."
    # Try common paths
    for path in \
        "$HOME/Qt/6.6.0/wasm_singlethread" \
        "$HOME/Qt/6.7.0/wasm_singlethread" \
        "/opt/Qt/6.6.0/wasm_singlethread" \
        "/opt/Qt/6.7.0/wasm_singlethread"
    do
        if [ -d "$path" ]; then
            QT_WASM_PATH="$path"
            echo "Found Qt WASM at: $QT_WASM_PATH"
            break
        fi
    done
fi

if [ -z "$QT_WASM_PATH" ] || [ ! -d "$QT_WASM_PATH" ]; then
    echo "ERROR: Qt WASM installation not found."
    echo "Please set QT_WASM_PATH environment variable to your Qt WASM installation."
    echo "Example: export QT_WASM_PATH=\$HOME/Qt/6.6.0/wasm_singlethread"
    exit 1
fi

# Check for Emscripten
if ! command -v emcc &> /dev/null; then
    echo "ERROR: Emscripten not found. Please install and activate Emscripten SDK."
    echo "See: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Create build directory
cd qt
mkdir -p build
cd build

# Configure with CMake
echo "Configuring Qt project..."
"$QT_WASM_PATH/bin/qt-cmake" .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . --parallel

# Copy outputs
echo "Copying build outputs..."
mkdir -p ../../dist

# Copy generated files (Qt WASM outputs)
for ext in wasm js html; do
    for file in *.${ext}; do
        if [ -f "$file" ]; then
            cp "$file" ../../dist/
        fi
    done
done

# Copy qtloader.js if present
if [ -f "qtloader.js" ]; then
    cp qtloader.js ../../dist/
fi

echo "Qt WASM build complete!"
echo "Output files in dist/:"
ls -la ../../dist/*.wasm ../../dist/*.js 2>/dev/null || echo "  (Qt WASM files)"
