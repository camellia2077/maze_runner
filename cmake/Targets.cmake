# --- Targets ---
add_library(maze_core STATIC
    libs/maze_core/domain/grid_topology.cpp
    libs/maze_core/domain/maze_generation.cpp
    libs/maze_core/domain/maze_generation_grid_ops.cpp
    libs/maze_core/domain/maze_generation_dfs.cpp
    libs/maze_core/domain/maze_generation_prims.cpp
    libs/maze_core/domain/maze_generation_kruskal.cpp
    libs/maze_core/domain/maze_generation_recursive_division.cpp
    libs/maze_core/domain/maze_generation_growing_tree.cpp
    libs/maze_core/domain/maze_generation_factory.cpp
    libs/maze_core/domain/maze_solver_grid_utils.cpp
    libs/maze_core/domain/maze_solver_neighbors.cpp
    libs/maze_core/domain/maze_solver_event_emitter.cpp
    libs/maze_core/domain/maze_solver_result_builder.cpp
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

set(MAZE_EXPORT_SOURCES
    libs/maze_infra/graphics/maze_renderer.cpp
    libs/maze_infra/graphics/render_layout.cpp
    libs/maze_infra/graphics/render_palette.cpp
    libs/maze_infra/graphics/render_raster.cpp
    libs/maze_infra/graphics/render_output_png.cpp
)

add_library(maze_export STATIC
    ${MAZE_EXPORT_SOURCES}
    libs/maze_export/export/maze_export.cpp
)

if(NOT ANDROID)
    add_library(maze_infra STATIC
        libs/maze_infra/config/config_loader.cpp
    )
else()
    add_library(maze_infra INTERFACE)
endif()

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

if(WIN32 AND MAZE_ENABLE_WIN_GUI)
    add_executable(maze_win_gui WIN32
        apps/maze_win_gui/src/main.cpp
        apps/maze_win_gui/src/event/maze_event_parser.cpp
        apps/maze_win_gui/src/state/gui_state_store.cpp
        apps/maze_win_gui/src/render/maze_rasterizer.cpp
        apps/maze_win_gui/src/runtime/session_runner.cpp
    )
endif()

add_executable(maze_generator_app ALIAS maze_cli)

target_include_directories(maze_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_core>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(maze_export
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_infra>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_export>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor
)

target_include_directories(maze_export SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

if(NOT ANDROID)
    target_include_directories(maze_infra
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_infra>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
else()
    target_include_directories(maze_infra
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_infra>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
endif()

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

if(TARGET maze_win_gui)
    target_include_directories(maze_win_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/maze_win_gui
    )
endif()

target_include_directories(maze_cli SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

target_link_libraries(maze_export PUBLIC maze_core)
if(NOT ANDROID)
    target_link_libraries(maze_infra PUBLIC maze_core)
else()
    target_link_libraries(maze_infra INTERFACE maze_core)
endif()
target_link_libraries(maze_usecase PUBLIC maze_core)
target_link_libraries(maze_api_c PUBLIC maze_usecase)
target_link_libraries(maze_cli PRIVATE maze_usecase maze_infra)
if(TARGET maze_win_gui)
    target_link_libraries(maze_win_gui PRIVATE maze_api_c user32 gdi32)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(maze_core PRIVATE -Wall -Wpedantic)
    target_compile_options(maze_export PRIVATE -Wall -Wpedantic)
    if(NOT ANDROID)
        target_compile_options(maze_infra PRIVATE -Wall -Wpedantic)
    endif()
    target_compile_options(maze_usecase PRIVATE -Wall -Wpedantic)
    target_compile_options(maze_api_c PRIVATE -Wall -Wpedantic)
    target_compile_options(maze_cli PRIVATE -Wall -Wpedantic)
elseif(MSVC)
    target_compile_options(maze_core PRIVATE /W4)
    target_compile_options(maze_export PRIVATE /W4)
    if(NOT ANDROID)
        target_compile_options(maze_infra PRIVATE /W4)
    endif()
    target_compile_options(maze_usecase PRIVATE /W4)
    target_compile_options(maze_api_c PRIVATE /W4)
    target_compile_options(maze_cli PRIVATE /W4)
    if(TARGET maze_win_gui)
        target_compile_options(maze_win_gui PRIVATE /W4)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(TARGET maze_win_gui)
        target_compile_options(maze_win_gui PRIVATE -Wall -Wpedantic)
    endif()
endif()
