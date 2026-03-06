cmake_minimum_required(VERSION 3.16)

add_library(maze_android_jni SHARED
    maze_jni_bridge.cpp
)

if(ANDROID)
    target_link_options(maze_android_jni PRIVATE
        "-Wl,-z,max-page-size=16384"
    )
endif()

target_link_libraries(maze_android_jni
    PRIVATE
        maze_api_c
)
