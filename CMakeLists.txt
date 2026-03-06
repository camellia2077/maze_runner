# CMake 3.16 是 target_precompile_headers 的最低版本要求
cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Enable compilation database" FORCE)
project(MazeGenerator VERSION 0.3.0 LANGUAGES CXX)
include(GNUInstallDirs)

set(MAZE_KERNEL_VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_core/MAZE_CORE_VERSION.txt")
if(NOT EXISTS "${MAZE_KERNEL_VERSION_FILE}")
    message(FATAL_ERROR "Kernel version file not found: ${MAZE_KERNEL_VERSION_FILE}")
endif()
file(READ "${MAZE_KERNEL_VERSION_FILE}" MAZE_KERNEL_VERSION_RAW)
string(STRIP "${MAZE_KERNEL_VERSION_RAW}" MAZE_KERNEL_VERSION)
if(MAZE_KERNEL_VERSION STREQUAL "")
    message(FATAL_ERROR "Kernel version is empty in ${MAZE_KERNEL_VERSION_FILE}")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_core/common/kernel_version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/common/kernel_version.hpp"
    @ONLY
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Options.cmake)
include(cmake/Compiler.cmake)
include(cmake/Targets.cmake)
include(cmake/Tests.cmake)
include(cmake/Install.cmake)
include(cmake/PostBuild.cmake)
include(cmake/Messages.cmake)
include(cmake/Tools.cmake)

if(ANDROID AND MAZE_ENABLE_ANDROID_JNI)
    add_subdirectory(platform/android/native)
endif()
