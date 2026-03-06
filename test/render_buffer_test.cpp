#include <filesystem>
#include <iostream>
#include <string>

#include "application/services/maze_generation.h"
#include "application/services/maze_runtime_context.h"
#include "application/services/maze_solver.h"
#include "infrastructure/graphics/maze_renderer.h"

namespace {

namespace fs = std::filesystem;

void ExpectTrue(bool condition, const char* message, int& failures) {
  if (!condition) {
    std::cerr << "[EXPECT_TRUE] " << message << "\n";
    ++failures;
  }
}

template <typename Type>
void ExpectEqual(const Type& left, const Type& right, const char* message,
                 int& failures) {
  if (!(left == right)) {
    std::cerr << "[EXPECT_EQUAL] " << message << "\n";
    ++failures;
  }
}

auto BuildTestConfig(const fs::path& output_dir) -> Config::AppConfig {
  Config::AppConfig config;
  config.maze.width = 6;
  config.maze.height = 6;
  config.maze.unit_pixels = 8;
  config.maze.start_node = {0, 0};
  config.maze.end_node = {5, 5};
  config.output_dir = output_dir.string();
  return config;
}

class FrameCaptureSink final : public MazeSolverDomain::ISearchEventSink {
 public:
  FrameCaptureSink(int height, int width) {
    frame_.visual_states_.assign(
        static_cast<size_t>(height),
        std::vector<MazeSolverDomain::SolverCellState>(
            static_cast<size_t>(width), MazeSolverDomain::SolverCellState::NONE));
  }

  void OnEvent(const MazeSolverDomain::SearchEvent& event) override {
    if (captured_ || event.type != MazeSolverDomain::SearchEventType::kProgress) {
      return;
    }
    for (const auto& delta : event.deltas) {
      if (delta.row < 0 ||
          delta.row >= static_cast<int>(frame_.visual_states_.size())) {
        continue;
      }
      if (delta.col < 0 || frame_.visual_states_.empty() ||
          delta.col >= static_cast<int>(frame_.visual_states_.front().size())) {
        continue;
      }
      frame_.visual_states_[delta.row][delta.col] = delta.state;
    }
    frame_.current_path_ = event.path;
    captured_ = !event.deltas.empty() || !event.path.empty();
  }

  auto ShouldCancel() const -> bool override { return false; }

  auto HasFrame() const -> bool { return captured_; }
  auto Frame() const -> const MazeSolverDomain::SearchFrame& { return frame_; }

 private:
  bool captured_ = false;
  MazeSolverDomain::SearchFrame frame_;
};

}  // namespace

auto RunRenderBufferTests() -> int {
  int failures = 0;

  const fs::path output_dir =
      fs::temp_directory_path() / "maze_generator_render_buffer_tests";
  std::error_code fs_error;
  fs::create_directories(output_dir, fs_error);

  Config::AppConfig config = BuildTestConfig(output_dir);
  MazeDomain::MazeGrid maze_grid(
      static_cast<size_t>(config.maze.height),
      MazeDomain::MazeGrid::value_type(static_cast<size_t>(config.maze.width)));
  const MazeDomain::GridTopology2D topology(
      MazeDomain::GridSize2D{.height = config.maze.height,
                             .width = config.maze.width});

  auto runtime_context = MazeApplication::BuildRuntimeContext2D(
      maze_grid, topology, MazeDomain::MazeAlgorithmType::DFS,
      MazeSolverDomain::SolverAlgorithmType::BFS, config);
  MazeGeneration::generate_maze_structure(runtime_context);
  FrameCaptureSink sink(config.maze.height, config.maze.width);
  MazeSolver::SolveOptions options;
  options.emit_stride = 1;
  options.emit_progress = true;
  const MazeSolverDomain::SearchResult result =
      MazeSolver::Solve(runtime_context, &sink, options);

  ExpectTrue(sink.HasFrame(), "Solver should emit at least one progress frame",
             failures);
  if (!sink.HasFrame()) {
    return failures;
  }

  MazeSolver::RenderBuffer buffer;
  std::string error;
  const bool render_ok = MazeSolver::RenderFrameToBuffer(
      sink.Frame(), maze_grid, config, buffer, error);
  ExpectTrue(render_ok, "RenderFrameToBuffer should succeed", failures);
  ExpectTrue(error.empty(), "RenderFrameToBuffer error should be empty",
             failures);
  ExpectTrue(buffer.width > 0 && buffer.height > 0 && buffer.channels == 3,
             "Render buffer dimensions/channels should be valid", failures);
  const auto expected_size = static_cast<size_t>(buffer.width) *
                             static_cast<size_t>(buffer.height) *
                             static_cast<size_t>(buffer.channels);
  ExpectEqual(buffer.pixels.size(), expected_size,
              "Render buffer pixel size should match dimensions", failures);

  const fs::path output_file = output_dir / "single_frame.png";
  error.clear();
  const bool write_ok =
      MazeSolver::WritePngFile(output_file.string(), buffer, error);
  ExpectTrue(write_ok, "WritePngFile should succeed", failures);
  ExpectTrue(error.empty(), "WritePngFile error should be empty", failures);
  ExpectTrue(fs::exists(output_file), "Written png file should exist", failures);

  return failures;
}
