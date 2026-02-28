include(CTest)

if(BUILD_TESTING)
    add_executable(maze_unit_tests
        test/test_main.cpp
        test/grid_topology_test.cpp
        test/maze_runtime_context_test.cpp
        test/config_loader_test.cpp
        test/cli_app_test.cpp
        test/solver_algorithms_test.cpp
        test/render_buffer_test.cpp
        test/maze_api_c_test.cpp
        apps/maze_cli/cli/framework/cli_app.cpp
        apps/maze_cli/cli/commands/generation_algorithms_command.cpp
        apps/maze_cli/cli/commands/search_algorithms_command.cpp
    )

    target_include_directories(maze_unit_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/maze_cli
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_core
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_infra
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_api_c
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(maze_unit_tests PRIVATE -Wall -Wpedantic)
    elseif(MSVC)
        target_compile_options(maze_unit_tests PRIVATE /W4)
    endif()

    target_link_libraries(maze_unit_tests PRIVATE maze_core maze_infra maze_api_c)

    add_test(NAME maze_unit_tests COMMAND maze_unit_tests)

    add_executable(maze_render_tests
        test/render_artifact_test.cpp
    )

    target_include_directories(maze_render_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/apps/maze_cli
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_core
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/maze_infra
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )

    target_include_directories(maze_render_tests SYSTEM PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(maze_render_tests PRIVATE -Wall -Wpedantic)
    elseif(MSVC)
        target_compile_options(maze_render_tests PRIVATE /W4)
    endif()

    target_link_libraries(maze_render_tests PRIVATE maze_core maze_infra)

    add_test(NAME maze_render_tests COMMAND maze_render_tests)
    set_tests_properties(maze_render_tests PROPERTIES LABELS "png;artifact")
endif()
