#!/bin/bash
# Build Rust WASM and generate JavaScript bindings
set -e

echo "Building Rust WASM..."

# Create output directories
mkdir -p dist/pkg

# Build Rust WASM
cargo build --target wasm32-unknown-unknown --release

# Generate JS bindings with wasm-bindgen
echo "Generating JavaScript bindings..."
wasm-bindgen target/wasm32-unknown-unknown/release/airgap_json_formatter.wasm \
    --out-dir dist/pkg \
    --typescript \
    --target web

# Optimize WASM size (DISABLED - wasm-opt 108-118 has table growth bug)
# See: https://github.com/AztecProtocol/aztec-packages/issues/12379
# if command -v wasm-opt &> /dev/null; then
#     echo "Optimizing WASM..."
#     wasm-opt -O3 dist/pkg/airgap_json_formatter_bg.wasm \
#         -o dist/pkg/airgap_json_formatter_bg.wasm
# fi
echo "Skipping wasm-opt (disabled due to version 108-118 bug)"

# Report sizes
echo ""
echo "Build complete!"
echo "WASM size:"
ls -lh dist/pkg/airgap_json_formatter_bg.wasm

if command -v gzip &> /dev/null; then
    gzip -k -f dist/pkg/airgap_json_formatter_bg.wasm
    echo "Gzipped size:"
    ls -lh dist/pkg/airgap_json_formatter_bg.wasm.gz
    rm dist/pkg/airgap_json_formatter_bg.wasm.gz
fi

echo ""
echo "Output files:"
ls -la dist/pkg/
