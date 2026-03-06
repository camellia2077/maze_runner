# --- Build options ---
option(MAZE_ENABLE_OPTIMIZATIONS "Enable release optimizations" ON)
set(MAZE_RELEASE_OPT_LEVEL "O3" CACHE STRING "Release optimization level")
set_property(CACHE MAZE_RELEASE_OPT_LEVEL PROPERTY STRINGS O0 O1 O2 O3)
set(MAZE_RELEASE_EXTRA_FLAGS "-march=native -flto" CACHE STRING
    "Extra Release compiler flags")
option(MAZE_RELEASE_STRIP_BINARIES "Strip release binaries" ON)
option(MAZE_FORCE_STATIC_LINK "Enable static linking for release executable" OFF)
option(MAZE_ENABLE_WIN_GUI "Build Windows GUI shell app" ON)
option(MAZE_ENABLE_ANDROID_JNI "Build Android JNI bridge module" OFF)

if(MAZE_ENABLE_OPTIMIZATIONS)
    set(CMAKE_CXX_FLAGS_RELEASE
        "-${MAZE_RELEASE_OPT_LEVEL} ${MAZE_RELEASE_EXTRA_FLAGS}")
    if(MAZE_RELEASE_STRIP_BINARIES)
        string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -s")
    endif()
else()
    message(STATUS "Release optimizations disabled.")
endif()

if(MAZE_FORCE_STATIC_LINK)
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY
            "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE
            " -static -static-libgcc -static-libstdc++")
    endif()
endif()
