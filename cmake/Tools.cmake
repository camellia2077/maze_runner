# --- Tooling targets ---

file(GLOB_RECURSE MAZE_ALL_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp
)

list(FILTER MAZE_ALL_SOURCES EXCLUDE REGEX "/vendor/")
list(FILTER MAZE_ALL_SOURCES EXCLUDE REGEX "/lib/")
list(FILTER MAZE_ALL_SOURCES EXCLUDE REGEX "[/\\\\]vendor[/\\\\]")
list(FILTER MAZE_ALL_SOURCES EXCLUDE REGEX "[/\\\\]lib[/\\\\]")

set(MAZE_TIDY_HEADER_FILTER "^${CMAKE_CURRENT_SOURCE_DIR}/(src|apps|libs|tests)/")

add_custom_target(format
    COMMAND clang-format -i ${MAZE_ALL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running clang-format"
)

add_custom_target(tidy
    COMMAND clang-tidy -p ${CMAKE_BINARY_DIR} -header-filter=${MAZE_TIDY_HEADER_FILTER} ${MAZE_ALL_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running clang-tidy"
)
