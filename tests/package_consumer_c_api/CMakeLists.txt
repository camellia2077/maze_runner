cmake_minimum_required(VERSION 3.16)
project(MazeGeneratorCApiConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MazeGenerator CONFIG REQUIRED)

add_executable(maze_c_api_consumer_smoke
    main.cpp
)

target_link_libraries(maze_c_api_consumer_smoke PRIVATE
    MazeGenerator::maze_api_c
)
