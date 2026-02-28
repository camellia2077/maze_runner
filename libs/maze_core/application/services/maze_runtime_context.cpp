#include "application/services/maze_runtime_context.h"

namespace MazeApplication {

auto MazeRuntimeContext::IsValid() const -> bool {
  if (topology_ == nullptr || maze_grid_ == nullptr || app_config_ == nullptr) {
    return false;
  }
  if (topology_->Dimension() != MazeDomain::MazeDimension::D2) {
    return false;
  }
  if (topology_->CellCount() <= 0) {
    return false;
  }
  const auto grid_height = static_cast<int>(maze_grid_->size());
  if (grid_height <= 0) {
    return false;
  }
  const auto grid_width = static_cast<int>(maze_grid_->front().size());
  return start_node_.first >= 0 && start_node_.first < grid_height &&
         start_node_.second >= 0 && start_node_.second < grid_width &&
         end_node_.first >= 0 && end_node_.first < grid_height &&
         end_node_.second >= 0 && end_node_.second < grid_width;
}

auto BuildRuntimeContext2D(MazeDomain::MazeGrid& maze_grid,
                           const MazeDomain::IGridTopology& topology,
                           MazeDomain::MazeAlgorithmType generation_algorithm,
                           MazeSolverDomain::SolverAlgorithmType solver_algorithm,
                           const Config::AppConfig& app_config)
    -> MazeRuntimeContext {
  MazeRuntimeContext context;
  context.topology_ = &topology;
  context.maze_grid_ = &maze_grid;
  context.start_node_ = app_config.maze.start_node;
  context.end_node_ = app_config.maze.end_node;
  context.generation_algorithm_ = generation_algorithm;
  context.solver_algorithm_ = solver_algorithm;
  context.app_config_ = &app_config;
  return context;
}

}  // namespace MazeApplication
