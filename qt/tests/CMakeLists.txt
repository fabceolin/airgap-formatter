cmake_minimum_required(VERSION 3.21)
project(AsyncSerialiserTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Enable strict compiler warnings (AC15: No compiler warnings with -Wall -Wextra)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Test Gui Quick)

enable_testing()

# AsyncSerialiser unit tests
qt_add_executable(tst_asyncserialiser
    tst_asyncserialiser.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
)

target_include_directories(tst_asyncserialiser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_asyncserialiser PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME tst_asyncserialiser COMMAND tst_asyncserialiser)

# JsonBridge async operations tests (Story 5.2)
qt_add_executable(tst_jsonbridge_async
    tst_jsonbridge_async.cpp
    ../asyncserialiser.cpp
    ../asyncserialiser.h
    ../jsonbridge.cpp
    ../jsonbridge.h
    ../qjsontreemodel.cpp
    ../qjsontreemodel.h
    ../qjsontreeitem.cpp
    ../qjsontreeitem.h
)

target_include_directories(tst_jsonbridge_async PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(tst_jsonbridge_async PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Gui
    Qt6::Quick
)

add_test(NAME tst_jsonbridge_async COMMAND tst_jsonbridge_async)
