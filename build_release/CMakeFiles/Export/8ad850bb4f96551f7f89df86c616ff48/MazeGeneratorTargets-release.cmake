#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MazeGenerator::maze_core" for configuration "Release"
set_property(TARGET MazeGenerator::maze_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(MazeGenerator::maze_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmaze_core.a"
  )

list(APPEND _cmake_import_check_targets MazeGenerator::maze_core )
list(APPEND _cmake_import_check_files_for_MazeGenerator::maze_core "${_IMPORT_PREFIX}/lib/libmaze_core.a" )

# Import target "MazeGenerator::maze_infra" for configuration "Release"
set_property(TARGET MazeGenerator::maze_infra APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(MazeGenerator::maze_infra PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmaze_infra.a"
  )

list(APPEND _cmake_import_check_targets MazeGenerator::maze_infra )
list(APPEND _cmake_import_check_files_for_MazeGenerator::maze_infra "${_IMPORT_PREFIX}/lib/libmaze_infra.a" )

# Import target "MazeGenerator::maze_cli" for configuration "Release"
set_property(TARGET MazeGenerator::maze_cli APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(MazeGenerator::maze_cli PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/maze_cli.exe"
  )

list(APPEND _cmake_import_check_targets MazeGenerator::maze_cli )
list(APPEND _cmake_import_check_files_for_MazeGenerator::maze_cli "${_IMPORT_PREFIX}/bin/maze_cli.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
