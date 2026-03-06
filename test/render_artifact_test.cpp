#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "application/services/maze_generation.h"
#include "application/services/maze_runtime_context.h"
#include "application/services/maze_solver.h"
#include "export/maze_export.h"

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

class FrameCollectorSink final : public MazeSolverDomain::ISearchEventSink {
 public:
  FrameCollectorSink(int height, int width) {
    current_frame_.visual_states_.assign(
        static_cast<size_t>(height),
        std::vector<MazeSolverDomain::SolverCellState>(
            static_cast<size_t>(width), MazeSolverDomain::SolverCellState::NONE));
  }

  void OnEvent(const MazeSolverDomain::SearchEvent& event) override {
    if (event.type != MazeSolverDomain::SearchEventType::kProgress) {
      return;
    }
    for (const auto& delta : event.deltas) {
      if (delta.row < 0 ||
          delta.row >= static_cast<int>(current_frame_.visual_states_.size())) {
        continue;
      }
      if (delta.col < 0 || current_frame_.visual_states_.empty() ||
          delta.col >=
              static_cast<int>(current_frame_.visual_states_.front().size())) {
        continue;
      }
      current_frame_.visual_states_[delta.row][delta.col] = delta.state;
    }
    current_frame_.current_path_ = event.path;
    if (event.deltas.empty() && event.path.empty()) {
      return;
    }
    frames_.push_back(current_frame_);
  }

  auto ShouldCancel() const -> bool override { return false; }

  auto Frames() const -> const std::vector<MazeSolverDomain::SearchFrame>& {
    return frames_;
  }

 private:
  MazeSolverDomain::SearchFrame current_frame_;
  std::vector<MazeSolverDomain::SearchFrame> frames_;
};

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
  FrameCollectorSink sink(config.maze.height, config.maze.width);
  MazeSolver::SolveOptions options;
  options.emit_stride = 1;
  options.emit_progress = true;
  MazeSolver::Solve(runtime_context, &sink, options);
  if (sink.Frames().empty()) {
    std::cerr << "[FAIL] No progress frames collected from solver.\n";
    return 1;
  }

  const fs::path output_root = ResolveOutputRoot();
  std::error_code fs_error;
  fs::create_directories(output_root, fs_error);
  if (fs_error) {
    std::cerr << "[FAIL] Failed to create output folder: "
              << output_root.string() << "\n";
    return 1;
  }
  const auto export_result = MazeExport::ExportFramesToPngSequence(
      sink.Frames(), maze_grid, config, "bfs/DFS");
  if (!export_result.ok) {
    std::cerr << "[FAIL] ExportFramesToPngSequence failed: "
              << export_result.error << "\n";
    return 1;
  }

  const fs::path output_folder = export_result.output_folder;
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
