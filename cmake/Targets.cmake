# --- Targets ---
add_library(maze_core STATIC
    libs/maze_core/domain/grid_topology.cpp
    libs/maze_core/domain/maze_generation.cpp
    libs/maze_core/domain/maze_solver_common.cpp
    libs/maze_core/domain/maze_solver.cpp
    libs/maze_core/domain/maze_solver_bfs.cpp
    libs/maze_core/domain/maze_solver_dfs.cpp
    libs/maze_core/domain/maze_solver_astar.cpp
    libs/maze_core/domain/maze_solver_dijkstra.cpp
    libs/maze_core/domain/maze_solver_greedy.cpp
    libs/maze_core/application/services/maze_generation.cpp
    libs/maze_core/application/services/maze_runtime_context.cpp
    libs/maze_core/application/services/maze_solver.cpp
)

add_library(maze_infra STATIC
    libs/maze_infra/config/config_loader.cpp
    libs/maze_infra/graphics/maze_renderer.cpp
)

add_library(maze_usecase STATIC
    libs/maze_usecase/usecase/maze_pipeline.cpp
)

add_library(maze_api_c STATIC
    libs/maze_api_c/api/maze_api.cpp
)

add_executable(maze_cli
    apps/maze_cli/src/main.cpp
    apps/maze_cli/cli/framework/cli_app.cpp
    apps/maze_cli/cli/commands/generation_algorithms_command.cpp
    apps/maze_cli/cli/commands/search_algorithms_command.cpp
    apps/maze_cli/cli/commands/version_command.cpp
)

add_executable(maze_generator_app ALIAS maze_cli)

target_include_directories(maze_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_core>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(maze_infra
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_infra>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor
)

target_include_directories(maze_infra SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

target_include_directories(maze_usecase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_usecase>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(maze_api_c
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_api_c>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(maze_cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/maze_cli
)

target_include_directories(maze_cli SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

target_link_libraries(maze_infra PUBLIC maze_core)
target_link_libraries(maze_usecase PUBLIC maze_core maze_infra)
target_link_libraries(maze_api_c PUBLIC maze_usecase)
target_link_libraries(maze_cli PRIVATE maze_usecase)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maze_core PRIVATE -Wall -Wpedantic)
    target_compile_options(maze_infra PRIVATE -Wall -Wpedantic)
    target_compile_options(maze_usecase PRIVATE -Wall -Wpedantic)
    target_compile_options(maze_api_c PRIVATE -Wall -Wpedantic)
    target_compile_options(maze_cli PRIVATE -Wall -Wpedantic)
elseif(MSVC)
    target_compile_options(maze_core PRIVATE /W4)
    target_compile_options(maze_infra PRIVATE /W4)
    target_compile_options(maze_usecase PRIVATE /W4)
    target_compile_options(maze_api_c PRIVATE /W4)
    target_compile_options(maze_cli PRIVATE /W4)
endif()
