cmake_minimum_required(VERSION 3.21)
project(AirgapJsonFormatter VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_add_executable(airgap_formatter
    main.cpp
    jsonbridge.cpp
    jsonbridge.h
    qjsontreeitem.cpp
    qjsontreeitem.h
    qjsontreemodel.cpp
    qjsontreemodel.h
    theme.cpp
    theme.h
)

qt_add_qml_module(airgap_formatter
    URI AirgapFormatter
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Header.qml
        qml/Toolbar.qml
        qml/InputPane.qml
        qml/OutputPane.qml
        qml/JsonTreeView.qml
        qml/StatusBar.qml
        qml/HistoryPanel.qml
    SOURCES
        theme.h
        theme.cpp
)

target_link_libraries(airgap_formatter PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)

# WebAssembly-specific settings
if(EMSCRIPTEN)
    # Generate ES6 module that exports a factory function
    target_link_options(airgap_formatter PRIVATE
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sEXPORT_NAME=createQtAppInstance
        -sENVIRONMENT=web
        -sASYNCIFY
        -sASYNCIFY_STACK_SIZE=131072
        -sALLOW_MEMORY_GROWTH=1
        --bind
    )
endif()

# Copy PWA assets to build directory for deployment
if(EMSCRIPTEN)
    file(COPY ${CMAKE_SOURCE_DIR}/../public/icon-192.png DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/../public/icon-512.png DESTINATION ${CMAKE_BINARY_DIR})
endif()
