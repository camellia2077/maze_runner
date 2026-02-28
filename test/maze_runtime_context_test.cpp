#include <iostream>
#include <vector>

#include "application/services/maze_runtime_context.h"

namespace {

void ExpectTrue(bool condition, const char* message, int& failures) {
  if (!condition) {
    std::cerr << "[EXPECT_TRUE] " << message << "\n";
    ++failures;
  }
}

class FakeTopology3D final : public MazeDomain::IGridTopology {
 public:
  [[nodiscard]] auto Dimension() const -> MazeDomain::MazeDimension override {
    return MazeDomain::MazeDimension::D3;
  }
  [[nodiscard]] auto CellCount() const -> int override { return 1; }
  [[nodiscard]] auto IsValidCell(MazeDomain::CellId cell_id) const
      -> bool override {
    return cell_id == 0;
  }
  [[nodiscard]] auto AdjacentCells(MazeDomain::CellId /*cell_id*/) const
      -> std::vector<MazeDomain::CellId> override {
    return {};
  }
};

auto CreateGrid(int height, int width) -> MazeDomain::MazeGrid {
  return {static_cast<size_t>(height),
          MazeDomain::MazeGrid::value_type(static_cast<size_t>(width))};
}

auto CreateConfig(int height, int width, std::pair<int, int> start_node,
                  std::pair<int, int> end_node) -> Config::AppConfig {
  Config::AppConfig config;
  config.maze.height = height;
  config.maze.width = width;
  config.maze.start_node = start_node;
  config.maze.end_node = end_node;
  return config;
}

}  // namespace

auto RunMazeRuntimeContextTests() -> int {
  int failures = 0;

  MazeDomain::MazeGrid maze_grid = CreateGrid(3, 3);
  const MazeDomain::GridTopology2D topology(
      MazeDomain::GridSize2D{.height = 3, .width = 3});
  const Config::AppConfig valid_config = CreateConfig(3, 3, {0, 0}, {2, 2});

  const MazeApplication::MazeRuntimeContext valid_context =
      MazeApplication::BuildRuntimeContext2D(
          maze_grid, topology, MazeDomain::MazeAlgorithmType::DFS,
          MazeSolverDomain::SolverAlgorithmType::BFS, valid_config);
  ExpectTrue(valid_context.IsValid(), "Valid context should pass", failures);

  const Config::AppConfig invalid_start_config =
      CreateConfig(3, 3, {-1, 0}, {2, 2});
  const MazeApplication::MazeRuntimeContext invalid_start_context =
      MazeApplication::BuildRuntimeContext2D(
          maze_grid, topology, MazeDomain::MazeAlgorithmType::DFS,
          MazeSolverDomain::SolverAlgorithmType::BFS, invalid_start_config);
  ExpectTrue(!invalid_start_context.IsValid(),
             "Context with invalid start should fail", failures);

  MazeApplication::MazeRuntimeContext null_context;
  ExpectTrue(!null_context.IsValid(), "Null context should fail", failures);

  const FakeTopology3D fake_topology;
  MazeApplication::MazeRuntimeContext mismatch_context;
  mismatch_context.topology_ = &fake_topology;
  mismatch_context.maze_grid_ = &maze_grid;
  mismatch_context.app_config_ = &valid_config;
  mismatch_context.start_node_ = {0, 0};
  mismatch_context.end_node_ = {2, 2};
  ExpectTrue(!mismatch_context.IsValid(),
             "3D topology should fail in 2D runtime context", failures);

  return failures;
}
