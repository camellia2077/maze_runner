#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "application/services/maze_generation.h"
#include "application/services/maze_runtime_context.h"
#include "application/services/maze_solver.h"
#include "infrastructure/graphics/maze_renderer.h"

namespace {

namespace fs = std::filesystem;

auto IsRenderEnabled() -> bool {
  const char* render_flag = std::getenv("MAZE_TEST_RENDER");
  return render_flag != nullptr && std::string(render_flag) == "1";
}

auto ResolveOutputRoot() -> fs::path {
  const char* custom_dir = std::getenv("MAZE_TEST_RENDER_DIR");
  if (custom_dir != nullptr && std::string(custom_dir).size() > 0) {
    return custom_dir;
  }
  return fs::path("test/artifacts/actual");
}

auto FileCountInFolder(const fs::path& folder_path) -> size_t {
  if (!fs::exists(folder_path)) {
    return 0;
  }
  size_t count = 0;
  for (const auto& entry : fs::directory_iterator(folder_path)) {
    if (entry.is_regular_file()) {
      ++count;
    }
  }
  return count;
}

}  // namespace

auto main() -> int {
  if (!IsRenderEnabled()) {
    std::cout << "[SKIP] Set MAZE_TEST_RENDER=1 to enable PNG artifact test.\n";
    return 0;
  }

  Config::AppConfig config;
  config.maze.width = 8;
  config.maze.height = 8;
  config.maze.unit_pixels = 12;
  config.maze.start_node = {0, 0};
  config.maze.end_node = {config.maze.height - 1, config.maze.width - 1};
  config.output_dir = ResolveOutputRoot().string();

  MazeGeneration::MazeGrid maze_grid(
      static_cast<size_t>(config.maze.height),
      MazeGeneration::MazeGrid::value_type(static_cast<size_t>(config.maze.width)));
  const MazeDomain::GridTopology2D topology(
      MazeDomain::GridSize2D{.height = config.maze.height,
                             .width = config.maze.width});

  auto runtime_context = MazeApplication::BuildRuntimeContext2D(
      maze_grid, topology, MazeGeneration::MazeAlgorithmType::DFS,
      MazeSolver::SolverAlgorithmType::BFS, config);
  MazeGeneration::generate_maze_structure(runtime_context);
  const MazeSolver::SearchResult result = MazeSolver::Solve(runtime_context);
  const auto render_result = MazeSolver::RenderSearchResult(
      result, maze_grid, runtime_context.solver_algorithm_,
      MazeGeneration::algorithm_name(runtime_context.generation_algorithm_),
      config);

  if (!render_result.ok) {
    std::cerr << "[FAIL] Render failed: " << render_result.error << "\n";
    return 1;
  }
  if (render_result.frames_written == 0) {
    std::cerr << "[FAIL] Render produced zero frames.\n";
    return 1;
  }

  const fs::path output_folder = render_result.output_folder;
  if (!fs::exists(output_folder)) {
    std::cerr << "[FAIL] Output folder not found: " << output_folder.string()
              << "\n";
    return 1;
  }
  const size_t file_count = FileCountInFolder(output_folder);
  if (file_count == 0) {
    std::cerr << "[FAIL] No files written to: " << output_folder.string()
              << "\n";
    return 1;
  }

  std::cout << "[PASS] PNG artifacts generated in: "
            << output_folder.string() << "\n";
  std::cout << "[PASS] Frame files: " << file_count << "\n";
  return 0;
}
