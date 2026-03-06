#include <exception>
#include <iostream>

#include "test_cases.h"

namespace {

auto RunSuite(const char* suite_name, auto (*runner)()) -> int {
  try {
    const int failure_count = runner();
    if (failure_count == 0) {
      std::cout << "[PASS] " << suite_name << "\n";
      return 0;
    }
    std::cout << "[FAIL] " << suite_name << " (" << failure_count
              << " failures)\n";
    return failure_count;
  } catch (const std::exception& error) {
    std::cerr << "[EXCEPTION] " << suite_name << ": " << error.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "[EXCEPTION] " << suite_name << ": unknown exception\n";
    return 1;
  }
}

}  // namespace

auto main() -> int {
  int total_failures = 0;
  total_failures += RunSuite("GridTopology2D", RunGridTopologyTests);
  total_failures +=
      RunSuite("MazeRuntimeContext", RunMazeRuntimeContextTests);
  total_failures += RunSuite("ConfigLoader", RunConfigLoaderTests);
  total_failures += RunSuite("CliApp", RunCliAppTests);
  total_failures += RunSuite("SolverAlgorithms", RunSolverAlgorithmTests);
  total_failures += RunSuite("RenderBuffer", RunRenderBufferTests);
  total_failures += RunSuite("MazeApiC", RunMazeApiCTests);
  total_failures += RunSuite("WinGuiParser", RunWinGuiParserTests);
  total_failures += RunSuite("WinGuiStateStore", RunWinGuiStateStoreTests);
  total_failures += RunSuite("WinGuiRasterizer", RunWinGuiRasterizerTests);
  total_failures += RunSuite("SolverEventEmitter", RunSolverEventEmitterTests);
  total_failures += RunSuite("SolverResultBuilder", RunSolverResultBuilderTests);

  if (total_failures == 0) {
    std::cout << "All tests passed.\n";
    return 0;
  }
  std::cout << "Test failures: " << total_failures << "\n";
  return 1;
}
