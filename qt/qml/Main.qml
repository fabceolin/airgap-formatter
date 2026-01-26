import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AirgapFormatter

ApplicationWindow {
    id: window
    visible: true
    width: 1280
    height: 800
    minimumWidth: 1024
    minimumHeight: 600
    title: "Airgap JSON Formatter"

    color: Theme.background

    // Track WASM initialization state
    property bool wasmInitialized: false

    // View mode: "tree" or "text"
    property string viewMode: "tree"

    // Store current formatted JSON for both views
    property string currentFormattedJson: ""

    // Track when JsonBridge becomes ready and handle async operation results
    Connections {
        target: JsonBridge
        function onReadyChanged() {
            wasmInitialized = JsonBridge.ready;
        }

        function onFormatCompleted(result) {
            if (result.success) {
                currentFormattedJson = result.result;
                outputPane.text = result.result;
                // Load tree model for tree view (synchronous)
                JsonBridge.loadTreeModel(result.result);
                // Update status bar from format result (avoid extra async validateJson call)
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
                // Stop pending validation - formatting already validated
                validationTimer.stop();
                // Save to history via AsyncSerialiser queue
                JsonBridge.saveToHistory(result.result);
                // Auto-expand tree view after model loads
                autoExpandTimer.restart();
            } else {
                currentFormattedJson = "";
                outputPane.text = "Error: " + result.error;
            }
        }

        function onMinifyCompleted(result) {
            if (result.success) {
                currentFormattedJson = result.result;
                outputPane.text = result.result;
                // Load tree model for tree view (synchronous)
                JsonBridge.loadTreeModel(result.result);
                // Update status bar (avoid extra async validateJson call)
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
                // Stop pending validation - minifying already validated
                validationTimer.stop();
                // Save to history via AsyncSerialiser queue
                JsonBridge.saveToHistory(result.result);
                // Switch to text mode for minified output
                window.viewMode = "text";
            } else {
                currentFormattedJson = "";
                outputPane.text = "Error: " + result.error;
            }
        }

        function onValidateCompleted(result) {
            statusBar.isValid = result.isValid;

            if (result.isValid) {
                statusBar.errorMessage = "";
                statusBar.errorLine = 0;
                statusBar.errorColumn = 0;
                statusBar.objectCount = result.stats.object_count || 0;
                statusBar.arrayCount = result.stats.array_count || 0;
                statusBar.stringCount = result.stats.string_count || 0;
                statusBar.numberCount = result.stats.number_count || 0;
                statusBar.booleanCount = result.stats.boolean_count || 0;
                statusBar.nullCount = result.stats.null_count || 0;
                statusBar.totalKeys = result.stats.total_keys || 0;
                statusBar.maxDepth = result.stats.max_depth || 0;
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
            } else if (result.error) {
                statusBar.errorMessage = result.error.message || "Unknown error";
                statusBar.errorLine = result.error.line || 1;
                statusBar.errorColumn = result.error.column || 1;
                inputPane.errorLine = result.error.line || 1;
                inputPane.errorMessage = result.error.message || "Unknown error";
            }
        }

        function onHistoryLoaded(entries) {
            // History panel will receive this via its own connection
        }

        function onHistorySaved(success, id) {
            // History saved callback - can be used for feedback if needed
        }

        function onClipboardRead(content) {
            if (content && content.length > 0) {
                if (window.pasteMode === "simple") {
                    // Simple paste - just put text in input
                    inputPane.text = content;
                } else {
                    // Auto-format paste
                    handlePastedContent(content);
                }
            }
            window.pasteMode = "auto"; // Reset to default
        }

        function onCopyCompleted(success) {
            if (success) {
                toolbar.copyButtonText = "Copied!";
                copyFeedbackTimer.restart();
            }
        }

        function onBusyChanged(busy) {
            // Can be used for UI loading states
        }
    }

    // Handle pasted content with auto-format
    function handlePastedContent(text) {
        // Check if we should auto-format: input is empty or fully selected
        const shouldAutoFormat = inputPane.text.trim() === "" ||
                                 inputPane.isFullySelected();

        if (shouldAutoFormat) {
            // Try to format the pasted content
            inputPane.text = text;  // Put original in input
            JsonBridge.formatJson(text, toolbar.selectedIndent);
            // Result comes via onFormatCompleted signal
        } else {
            // Has partial content - paste normally without auto-format
            inputPane.text = text;
        }
    }

    // Initialize WASM on component completion
    Component.onCompleted: {
        // Check if bridge is ready via C++ context property
        wasmInitialized = JsonBridge.ready;
    }

    // Timer for copy feedback reset
    Timer {
        id: copyFeedbackTimer
        interval: 1500
        onTriggered: toolbar.copyButtonText = "Copy"
    }

    // Debounce timer for validation
    Timer {
        id: validationTimer
        interval: 300
        onTriggered: validateInput()
    }

    // Timer for auto-expand after model load
    Timer {
        id: autoExpandTimer
        interval: 50
        onTriggered: jsonTreeView.expandAll()
    }


    function validateInput() {
        if (!inputPane.text || !inputPane.text.trim()) {
            statusBar.isValid = true;
            statusBar.errorMessage = "";
            statusBar.objectCount = 0;
            statusBar.arrayCount = 0;
            statusBar.totalKeys = 0;
            statusBar.maxDepth = 0;
            inputPane.errorLine = -1;
            inputPane.errorMessage = "";
            return;
        }

        // Async call - result comes via onValidateCompleted signal
        JsonBridge.validateJson(inputPane.text);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Header {
            id: header
            Layout.fillWidth: true
            offlineReady: window.wasmInitialized
        }

        Toolbar {
            id: toolbar
            Layout.fillWidth: true
            viewMode: window.viewMode

            onViewModeToggled: {
                window.viewMode = (window.viewMode === "tree") ? "text" : "tree"
            }

            onExpandAllRequested: {
                if (window.viewMode === "tree") {
                    jsonTreeView.expandAll()
                }
            }

            onCollapseAllRequested: {
                if (window.viewMode === "tree") {
                    jsonTreeView.collapseAll()
                }
            }

            onFormatRequested: (indentType) => {
                if (!inputPane.text.trim()) {
                    return;
                }
                // Async call - result comes via onFormatCompleted signal
                JsonBridge.formatJson(inputPane.text, indentType);
            }

            onMinifyRequested: {
                if (!inputPane.text.trim()) {
                    return;
                }
                // Async call - result comes via onMinifyCompleted signal
                JsonBridge.minifyJson(inputPane.text);
            }

            onCopyRequested: {
                if (currentFormattedJson) {
                    // Async call - result comes via onCopyCompleted signal
                    JsonBridge.copyToClipboard(currentFormattedJson);
                }
            }

            onClearRequested: {
                inputPane.text = "";
                outputPane.text = "";
                currentFormattedJson = "";
                // Clear tree model
                JsonBridge.treeModel.clear();
                // Reset validation state
                statusBar.isValid = true;
                statusBar.errorMessage = "";
                statusBar.objectCount = 0;
                statusBar.arrayCount = 0;
                statusBar.totalKeys = 0;
                statusBar.maxDepth = 0;
                inputPane.errorLine = -1;
                inputPane.errorMessage = "";
            }

            onLoadHistoryRequested: {
                historyPanel.open();
            }
        }

        SplitView {
            id: splitView
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: window.width >= 1200 ? Qt.Horizontal : Qt.Vertical

            handle: Rectangle {
                implicitWidth: 6
                implicitHeight: 6
                color: SplitHandle.hovered ? Theme.accent : Theme.splitHandle
            }

            InputPane {
                id: inputPane
                SplitView.preferredWidth: parent.width / 2
                SplitView.preferredHeight: parent.height / 2
                SplitView.minimumWidth: 300
                SplitView.minimumHeight: 200

                onTextChanged: {
                    validationTimer.restart();
                }
            }

            // Output area - switches between TreeView and TextArea
            Item {
                id: outputArea
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumWidth: 300
                SplitView.minimumHeight: 200

                OutputPane {
                    id: outputPane
                    anchors.fill: parent
                    visible: viewMode === "text"
                }

                JsonTreeView {
                    id: jsonTreeView
                    anchors.fill: parent
                    visible: viewMode === "tree"
                    model: JsonBridge.treeModel
                }
            }
        }

        StatusBar {
            id: statusBar
            Layout.fillWidth: true
        }
    }

    // History panel (drawer from right)
    HistoryPanel {
        id: historyPanel
        parent: Overlay.overlay

        onEntrySelected: (content) => {
            // Stop any pending validation - format will re-validate
            validationTimer.stop();
            inputPane.text = content;
            // Direct call - AsyncSerialiser handles operation serialization
            JsonBridge.formatJson(content, toolbar.selectedIndent);
        }
    }

    // Keyboard shortcuts
    Shortcut {
        sequence: "Ctrl+Shift+F"
        onActivated: toolbar.formatRequested(toolbar.selectedIndent)
    }

    Shortcut {
        sequence: "Ctrl+Shift+M"
        onActivated: toolbar.minifyRequested()
    }

    Shortcut {
        sequence: "Ctrl+Shift+C"
        onActivated: toolbar.copyRequested()
    }

    Shortcut {
        sequence: "Ctrl+Shift+Ins"
        onActivated: {
            // Async call - result comes via onClipboardRead signal
            // Set a flag so we know this is a simple paste (no auto-format)
            window.pasteMode = "simple";
            JsonBridge.readFromClipboard();
        }
    }

    // Track paste mode for clipboard read handler
    property string pasteMode: "auto"

    // Ctrl+V paste with auto-format (Qt WASM doesn't handle browser paste events well)
    Shortcut {
        sequences: [StandardKey.Paste]
        onActivated: {
            // Async call - result comes via onClipboardRead signal
            window.pasteMode = "auto";
            JsonBridge.readFromClipboard();
        }
    }

    // TreeView expand/collapse shortcuts
    Shortcut {
        sequence: "Ctrl+E"
        enabled: viewMode === "tree"
        onActivated: jsonTreeView.expandAll()
    }

    Shortcut {
        sequence: "Ctrl+Shift+E"
        enabled: viewMode === "tree"
        onActivated: jsonTreeView.collapseAll()
    }

    // Toggle view mode
    Shortcut {
        sequence: "Ctrl+T"
        onActivated: viewMode = (viewMode === "tree") ? "text" : "tree"
    }

    // Open history panel
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: historyPanel.open()
    }
}
