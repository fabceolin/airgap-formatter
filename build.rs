//! Build script - minimal, no binary generation needed
//!
//! The JSON highlighter now uses a simple state machine parser
//! instead of syntect, avoiding WASM binary compatibility issues.

fn main() {
    // No binary generation needed anymore
    // The highlighter uses pure Rust code without external binary dependencies
    println!("cargo:rerun-if-changed=build.rs");
}
