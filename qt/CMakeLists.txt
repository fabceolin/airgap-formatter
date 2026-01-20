cmake_minimum_required(VERSION 3.21)
project(AirgapJsonFormatter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_add_executable(airgap_formatter
    main.cpp
)

# Mark Theme.qml as singleton
set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(airgap_formatter
    URI AirgapFormatter
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Header.qml
        qml/Toolbar.qml
        qml/InputPane.qml
        qml/OutputPane.qml
        qml/StatusBar.qml
)

target_link_libraries(airgap_formatter PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)

# WebAssembly-specific settings
if(EMSCRIPTEN)
    # Use .js output to avoid EXPORT_NAME + HTML shell conflict
    # We provide our own index.html that loads the Qt WASM module
    set_target_properties(airgap_formatter PROPERTIES
        SUFFIX ".js"
    )

    # Required link options for Qt WASM
    target_link_options(airgap_formatter PRIVATE
        -sEXPORT_NAME=createQtAppInstance
        -sASYNCIFY
    )
endif()
