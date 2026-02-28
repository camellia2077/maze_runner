# CMake generated Testfile for 
# Source directory: C:/Computer/my_github/github_cpp/maze/Maze_Generator
# Build directory: C:/Computer/my_github/github_cpp/maze/Maze_Generator/build_release
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(maze_unit_tests "C:/Computer/my_github/github_cpp/maze/Maze_Generator/build_release/maze_unit_tests.exe")
set_tests_properties(maze_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Computer/my_github/github_cpp/maze/Maze_Generator/cmake/Tests.cmake;31;add_test;C:/Computer/my_github/github_cpp/maze/Maze_Generator/cmake/Tests.cmake;0;;C:/Computer/my_github/github_cpp/maze/Maze_Generator/CMakeLists.txt;15;include;C:/Computer/my_github/github_cpp/maze/Maze_Generator/CMakeLists.txt;0;")
add_test(maze_render_tests "C:/Computer/my_github/github_cpp/maze/Maze_Generator/build_release/maze_render_tests.exe")
set_tests_properties(maze_render_tests PROPERTIES  LABELS "png;artifact" _BACKTRACE_TRIPLES "C:/Computer/my_github/github_cpp/maze/Maze_Generator/cmake/Tests.cmake;56;add_test;C:/Computer/my_github/github_cpp/maze/Maze_Generator/cmake/Tests.cmake;0;;C:/Computer/my_github/github_cpp/maze/Maze_Generator/CMakeLists.txt;15;include;C:/Computer/my_github/github_cpp/maze/Maze_Generator/CMakeLists.txt;0;")
