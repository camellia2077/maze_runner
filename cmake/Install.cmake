include(CMakePackageConfigHelpers)

set(MAZE_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/MazeGenerator")
set(MAZE_PACKAGE_VERSION "${PROJECT_VERSION}")
if(MAZE_PACKAGE_VERSION STREQUAL "")
    set(MAZE_PACKAGE_VERSION "0.0.0")
endif()

set(MAZE_PUBLIC_HEADERS
    libs/maze_api_c/api/maze_api.h
    libs/maze_export/export/maze_export.h
    libs/maze_usecase/usecase/maze_pipeline.h
    libs/maze_core/application/services/maze_generation.h
    libs/maze_core/application/services/maze_runtime_context.h
    libs/maze_core/application/services/maze_solver.h
    libs/maze_core/config/config.h
    libs/maze_core/domain/grid_topology.h
    libs/maze_core/domain/maze_generation.h
    libs/maze_core/domain/maze_grid.h
    libs/maze_core/domain/maze_solver.h
    libs/maze_core/domain/maze_solver_algorithms.h
    libs/maze_core/domain/maze_solver_common.h
    libs/maze_infra/infrastructure/config/config_loader.h
    libs/maze_infra/infrastructure/graphics/maze_renderer.h
)

install(
    TARGETS maze_core maze_export maze_infra maze_usecase maze_api_c maze_cli
    EXPORT MazeGeneratorTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

foreach(header_path IN LISTS MAZE_PUBLIC_HEADERS)
    get_filename_component(header_dir "${header_path}" DIRECTORY)
    string(REGEX REPLACE "^libs/[^/]+/" "" header_subdir "${header_dir}")
    install(
        FILES ${header_path}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${header_subdir}"
    )
endforeach()

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/common/kernel_version.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/common"
)

install(
    EXPORT MazeGeneratorTargets
    FILE MazeGeneratorTargets.cmake
    NAMESPACE MazeGenerator::
    DESTINATION ${MAZE_INSTALL_CMAKE_DIR}
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MazeGeneratorConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MazeGeneratorConfig.cmake
    INSTALL_DESTINATION ${MAZE_INSTALL_CMAKE_DIR}
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/MazeGeneratorConfigVersion.cmake
    VERSION ${MAZE_PACKAGE_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/MazeGeneratorConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/MazeGeneratorConfigVersion.cmake
    DESTINATION ${MAZE_INSTALL_CMAKE_DIR}
)
