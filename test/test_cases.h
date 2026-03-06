#ifndef MAZE_TEST_TEST_CASES_H
#define MAZE_TEST_TEST_CASES_H

auto RunGridTopologyTests() -> int;
auto RunMazeRuntimeContextTests() -> int;
auto RunConfigLoaderTests() -> int;
auto RunCliAppTests() -> int;
auto RunSolverAlgorithmTests() -> int;
auto RunRenderBufferTests() -> int;
auto RunMazeApiCTests() -> int;
auto RunWinGuiParserTests() -> int;
auto RunWinGuiStateStoreTests() -> int;
auto RunWinGuiRasterizerTests() -> int;
auto RunSolverEventEmitterTests() -> int;
auto RunSolverResultBuilderTests() -> int;

#endif  // MAZE_TEST_TEST_CASES_H
