cmake_minimum_required(VERSION 3.21)

project(SimpleDualSheath VERSION 1.6.0 LANGUAGES CXX)

set(BUILD_TESTS OFF CACHE BOOL "Disable CommonLibSSE-NG unit tests" FORCE)

add_subdirectory(lib/commonlibsse-ng CommonLibSSE EXCLUDE_FROM_ALL)
include(lib/commonlibsse-ng/cmake/CommonLibSSE.cmake)

add_commonlibsse_plugin(${PROJECT_NAME}
    SOURCES
        src/Common.cpp
        src/Config.cpp
        src/Controller.cpp
        src/Data.cpp
        src/EngineExtensions.cpp
        src/EquipManager.cpp
        src/InputHandler.cpp
        src/NodeUtils.cpp
        src/PluginInterface.cpp
        src/StringHolder.cpp
        src/Tasks.cpp
        src/main.cpp
    NAME "SimpleDualSheath"
    AUTHOR "SlavicPotato"
    VERSION ${PROJECT_VERSION}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    /Volumes/S512/Projects/ETC/vcpkg/installed/x64-windows-cross/include
)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
