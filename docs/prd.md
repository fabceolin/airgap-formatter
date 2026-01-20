# Airgap JSON Formatter - Product Requirements Document (PRD)

## 1. Goals and Background Context

### 1.1 Goals

- Provide developers with a secure, privacy-first JSON formatting tool that processes data entirely client-side
- Enable offline JSON manipulation after initial application load with zero network dependencies
- Deliver desktop-grade performance and UX through Rust/WASM with Qt interface in the browser
- Support handling of sensitive JSON payloads (API keys, PII, credentials) without data leaving the device
- Deploy as a static site on GitHub Pages for easy access and zero backend maintenance

### 1.2 Background Context

Developers frequently need to format, validate, and inspect JSON data containing sensitive information such as API keys, authentication tokens, personally identifiable information (PII), and production credentials. Existing online JSON tools pose significant privacy and security risks as they may transmit data to external servers, log inputs, or be subject to man-in-the-middle attacks.

Airgap JSON Formatter addresses this gap by providing a web-accessible tool that executes 100% client-side using WebAssembly. Once the WASM binary loads, the application makes zero external API calls - users can even disconnect from the internet and continue working. This architecture combines the convenience of web-based tools with the security guarantees of offline desktop applications.

### 1.3 Change Log

| Date | Version | Description | Author |
|------|---------|-------------|--------|
| 2025-01-20 | 1.0 | Initial PRD creation | Sarah (PO) |

---

## 2. Requirements

### 2.1 Functional Requirements

- **FR1:** The application shall accept JSON input via a text input area where users can paste or type JSON content
- **FR2:** The application shall format/prettify JSON with configurable indentation (2 spaces, 4 spaces, tabs)
- **FR3:** The application shall minify JSON by removing all unnecessary whitespace
- **FR4:** The application shall validate JSON syntax and display clear error messages with line/column positions for invalid JSON
- **FR5:** The application shall highlight syntax errors visually in the input area
- **FR6:** The application shall provide a copy-to-clipboard function for formatted output
- **FR7:** The application shall display the formatted/validated JSON in a read-only output area
- **FR8:** The application shall show JSON statistics (object count, array count, string count, total keys, nesting depth)
- **FR9:** The application shall support JSON files up to 10MB in size without significant performance degradation

### 2.2 Non-Functional Requirements

- **NFR1:** Zero network communication after initial WASM binary load - all processing must occur client-side
- **NFR2:** Application must function fully offline after initial load (service worker caching)
- **NFR3:** Initial load time shall be under 5 seconds on standard broadband connections
- **NFR4:** JSON formatting operations shall complete in under 100ms for files up to 1MB
- **NFR5:** Memory usage shall not exceed 256MB for typical operations (files under 5MB)
- **NFR6:** Application shall work on modern browsers (Chrome 90+, Firefox 88+, Safari 14+, Edge 90+)
- **NFR7:** UI shall be responsive and usable on screen widths from 1024px to 4K displays
- **NFR8:** All user data shall remain in browser memory only - no localStorage, IndexedDB, or cookie storage of JSON content
- **NFR9:** Source code shall be auditable - no obfuscation or minification that prevents security review

---

## 3. User Interface Design Goals

### 3.1 Overall UX Vision

A clean, professional, developer-focused interface that prioritizes functionality and clarity over visual flair. The design should feel like a native desktop application running in the browser - responsive, fast, and distraction-free. Security and privacy messaging should be subtle but always visible to reinforce trust.

### 3.2 Key Interaction Paradigms

- **Split-pane layout:** Input on left, output on right (or top/bottom on narrower screens)
- **Instant feedback:** Validation occurs as user types or pastes, with real-time error highlighting
- **Minimal clicks:** Primary actions (format, minify, copy) accessible via prominent buttons and keyboard shortcuts
- **Non-destructive:** Original input is never modified; output appears in separate pane

### 3.3 Core Screens and Views

- **Main Editor View:** The primary (and only) screen containing:
  - Input text area with syntax highlighting
  - Output text area (read-only) with syntax highlighting
  - Toolbar with format/minify/copy actions and indentation settings
  - Status bar showing validation state, error details, and JSON statistics
  - Privacy indicator showing "Offline Mode" / "Zero Network" status

### 3.4 Accessibility: WCAG AA

The application shall meet WCAG 2.1 AA standards including:
- Sufficient color contrast ratios
- Keyboard navigation for all functions
- Screen reader compatibility for status messages and errors
- Focus indicators for interactive elements

### 3.5 Branding

- **Color Scheme:** Professional dark theme as default (reduces eye strain for developers), with light theme option
- **Typography:** Monospace font for JSON content (system defaults: Consolas, Monaco, monospace)
- **Visual Identity:** Minimal, tool-focused - no mascots or decorative elements
- **Trust Indicators:** Subtle lock icon and "Airgap Protected" badge visible at all times

### 3.6 Target Device and Platforms: Web Responsive (Desktop-focused)

- Primary target: Desktop browsers (1024px+)
- Secondary: Tablet landscape mode
- Not optimized for: Mobile phones (JSON editing on small screens is impractical)

---

## 4. Technical Assumptions

### 4.1 Repository Structure: Monorepo

Single repository containing all components:
- `/src` - Rust source code
- `/qt` - Qt/QML UI definitions
- `/wasm` - WASM build outputs
- `/docs` - Documentation and PRD
- `/public` - Static assets for GitHub Pages

**Rationale:** Simple project with tightly coupled components; monorepo simplifies build pipeline and versioning.

### 4.2 Service Architecture

**Architecture: Single WASM Module (Client-Side Monolith)**

- One compiled WASM binary containing all Rust logic
- Qt framework compiled to WASM for UI rendering
- No backend services - GitHub Pages serves static files only
- Service Worker for offline caching

**Rationale:** Zero-network requirement eliminates need for backend; WASM provides native performance for JSON processing.

### 4.3 Testing Requirements

**Testing Strategy: Unit + Integration**

- **Unit Tests:** Rust unit tests for JSON parsing, formatting, validation logic
- **Integration Tests:** Browser-based tests verifying WASM/Qt integration
- **Manual Testing:** Cross-browser verification checklist
- **No E2E Framework:** Complexity not justified for single-page application

**Rationale:** Core JSON logic must be thoroughly tested; UI testing is manual due to Qt/WASM complexity.

### 4.4 Additional Technical Assumptions and Requests

- **Language:** Rust (stable toolchain)
- **UI Framework:** Qt 6 with QML for declarative UI
- **WASM Target:** `wasm32-unknown-unknown`
- **Build Tool:** Cargo with wasm-pack or similar for WASM compilation
- **Qt WASM:** Qt for WebAssembly (official Qt WASM support)
- **JSON Library:** `serde_json` for Rust JSON handling
- **Deployment:** GitHub Pages with GitHub Actions for CI/CD
- **Caching:** Service Worker using Workbox or minimal custom implementation
- **No External Dependencies:** No CDN resources, analytics, or third-party scripts after initial load

---

## 5. Epic List

### Epic 1: Project Foundation & Core JSON Engine
Establish project infrastructure, build pipeline, and implement core Rust JSON processing logic with basic WASM compilation.

### Epic 2: Qt UI Integration & Full Application
Integrate Qt WASM UI with Rust backend, implement the complete user interface, and deploy to GitHub Pages with offline support.

---

## 6. Epic Details

### Epic 1: Project Foundation & Core JSON Engine

**Goal:** Establish the foundational project structure with working Rust-to-WASM compilation, implement the core JSON processing library, and deploy a minimal "hello world" verification to GitHub Pages. This epic delivers a working build pipeline and validated JSON engine that Epic 2 will integrate with the Qt UI.

#### Story 1.1: Project Setup and Build Pipeline

**As a** developer,
**I want** a properly configured Rust project with WASM compilation support,
**so that** I can build and iterate on the application with confidence.

**Acceptance Criteria:**
1. Cargo.toml configured with appropriate dependencies (serde, serde_json, wasm-bindgen)
2. WASM target (`wasm32-unknown-unknown`) builds successfully via `cargo build --target wasm32-unknown-unknown`
3. Project structure follows Rust conventions with `src/lib.rs` as WASM entry point
4. `.gitignore` configured for Rust/WASM artifacts
5. Basic CI workflow (GitHub Actions) that builds the WASM target on push
6. README updated with build instructions

#### Story 1.2: Core JSON Formatting Module

**As a** user,
**I want** to format JSON with configurable indentation,
**so that** I can read and understand complex JSON structures.

**Acceptance Criteria:**
1. Rust module `json_formatter` implements `format_json(input: &str, indent: IndentStyle) -> Result<String, FormatError>`
2. `IndentStyle` enum supports `Spaces(u8)` and `Tabs` variants
3. Function correctly formats valid JSON with proper indentation and newlines
4. Function returns descriptive error with line/column position for invalid JSON
5. Unit tests cover: valid JSON formatting, various indent styles, nested objects/arrays, edge cases (empty object, empty array, unicode)
6. Performance benchmark confirms <100ms for 1MB JSON input

#### Story 1.3: JSON Validation and Statistics Module

**As a** user,
**I want** to validate JSON and see statistics about its structure,
**so that** I can quickly verify correctness and understand complexity.

**Acceptance Criteria:**
1. Rust module `json_validator` implements `validate_json(input: &str) -> ValidationResult`
2. `ValidationResult` contains: `is_valid: bool`, `error: Option<ValidationError>`, `stats: JsonStats`
3. `ValidationError` includes `message`, `line`, `column` fields
4. `JsonStats` includes: `object_count`, `array_count`, `string_count`, `number_count`, `boolean_count`, `null_count`, `max_depth`, `total_keys`
5. Unit tests cover: valid JSON validation, various error types with correct positions, statistics accuracy
6. Minify function `minify_json(input: &str) -> Result<String, FormatError>` implemented and tested

#### Story 1.4: WASM Bindings and JavaScript Interface

**As a** frontend developer,
**I want** clean JavaScript bindings to the Rust JSON functions,
**so that** the Qt UI can easily invoke the core logic.

**Acceptance Criteria:**
1. `wasm-bindgen` annotations expose `format_json`, `validate_json`, `minify_json` to JavaScript
2. TypeScript type definitions generated for all exposed functions
3. JavaScript-friendly wrapper handles string conversion and error serialization
4. Integration test verifies round-trip: JS call -> WASM -> JS result
5. Built WASM module size is under 2MB (gzipped under 500KB)
6. Example HTML page demonstrates calling WASM functions (temporary, for verification)

---

### Epic 2: Qt UI Integration & Full Application

**Goal:** Build the complete user interface using Qt for WebAssembly, integrate it with the Rust/WASM JSON engine from Epic 1, implement all user-facing features, and deploy the production application to GitHub Pages with offline support. This epic delivers the fully functional Airgap JSON Formatter.

#### Story 2.1: Qt WASM Project Setup and Integration

**As a** developer,
**I want** Qt for WebAssembly configured and integrated with the Rust WASM module,
**so that** I can build the UI with access to the JSON processing functions.

**Acceptance Criteria:**
1. Qt 6 project configured for WebAssembly target
2. QML files organized in `/qt/qml` directory
3. Rust WASM module successfully imported and callable from Qt/JavaScript layer
4. Basic "Hello Airgap" window renders in browser
5. Build script compiles both Rust WASM and Qt WASM components
6. GitHub Actions CI updated to build full application

#### Story 2.2: Main Editor UI Layout

**As a** user,
**I want** a split-pane editor interface,
**so that** I can see my input JSON and formatted output side by side.

**Acceptance Criteria:**
1. Main window with horizontal split: input pane (left) and output pane (right)
2. Input pane contains editable text area with monospace font
3. Output pane contains read-only text area with monospace font
4. Resizable split handle allows adjusting pane proportions
5. Window is responsive: switches to vertical split (top/bottom) below 1200px width
6. Dark theme applied as default with professional color scheme

#### Story 2.3: Toolbar and Actions

**As a** user,
**I want** clear action buttons and formatting options,
**so that** I can quickly format, minify, and copy JSON.

**Acceptance Criteria:**
1. Toolbar contains: Format button, Minify button, Copy Output button, Clear button
2. Indentation dropdown with options: 2 spaces, 4 spaces (default), tabs
3. Format button triggers `format_json` with selected indentation, displays result in output pane
4. Minify button triggers `minify_json`, displays result in output pane
5. Copy button copies output pane content to clipboard with visual feedback
6. Clear button resets both input and output panes
7. Keyboard shortcuts: Ctrl+Shift+F (format), Ctrl+Shift+M (minify), Ctrl+Shift+C (copy output)

#### Story 2.4: Validation Feedback and Error Display

**As a** user,
**I want** immediate feedback on JSON validity with clear error messages,
**so that** I can quickly identify and fix syntax errors.

**Acceptance Criteria:**
1. Status bar at bottom shows validation state: "Valid JSON" (green) or "Invalid JSON" (red)
2. On invalid JSON, error message displays with line and column number
3. Input pane scrolls to and highlights the error line
4. Validation runs automatically on input change (debounced 300ms)
5. Statistics display in status bar when JSON is valid (key count, depth, etc.)
6. Error tooltip appears when hovering over highlighted error line

#### Story 2.5: Privacy Indicators and Trust UI

**As a** user,
**I want** visible confirmation that my data stays local,
**so that** I can trust the application with sensitive JSON.

**Acceptance Criteria:**
1. Header bar displays "Airgap Protected" badge with lock icon
2. Network status indicator shows "Offline Ready" after initial load
3. Subtle footer text: "All processing happens locally. Your data never leaves this device."
4. No loading spinners or network activity indicators after initial load
5. Console logs confirm zero network requests during operation (verifiable in DevTools)

#### Story 2.6: Service Worker and Offline Support

**As a** user,
**I want** the application to work offline,
**so that** I can use it without an internet connection after first visit.

**Acceptance Criteria:**
1. Service Worker registered on first load
2. All application assets (HTML, JS, WASM, CSS) cached for offline use
3. Application loads and functions fully when offline
4. Cache versioning implemented for updates (new version replaces old cache)
5. User notification when new version is available (optional refresh prompt)
6. Cache size remains under 10MB total

#### Story 2.7: GitHub Pages Deployment

**As a** user,
**I want** to access the application via a public URL,
**so that** I can use it from any browser without installation.

**Acceptance Criteria:**
1. GitHub Actions workflow builds and deploys to GitHub Pages on main branch push
2. Custom 404.html handles SPA routing (if needed)
3. Correct MIME types configured for WASM files
4. Application loads successfully from `https://<username>.github.io/airgap-json-formatter/`
5. All assets load over HTTPS with no mixed content warnings
6. README updated with live demo link

#### Story 2.8: Cross-Browser Testing and Polish

**As a** user,
**I want** the application to work reliably across browsers,
**so that** I can use my preferred browser.

**Acceptance Criteria:**
1. Application tested and functional in Chrome 90+, Firefox 88+, Safari 14+, Edge 90+
2. Keyboard navigation works for all interactive elements
3. Focus indicators visible on all focusable elements
4. Color contrast meets WCAG AA standards (verified with contrast checker)
5. No console errors during normal operation
6. Performance profiling confirms <100ms format time for 1MB JSON in all browsers

---

## 7. Checklist Results Report

*To be completed after PRD review*

---

## 8. Next Steps

### 8.1 UX Expert Prompt

> Review the Airgap JSON Formatter PRD at `docs/prd.md`. Create detailed wireframes and UI specifications for the single-page editor interface, focusing on the split-pane layout, toolbar design, error display patterns, and privacy trust indicators. Ensure WCAG AA compliance in all designs.

### 8.2 Architect Prompt

> Review the Airgap JSON Formatter PRD at `docs/prd.md`. Create the technical architecture document covering: Rust module structure for JSON processing, WASM build configuration, Qt for WebAssembly integration patterns, Service Worker caching strategy, and GitHub Actions CI/CD pipeline. Ensure the architecture enforces the zero-network constraint after initial load.
