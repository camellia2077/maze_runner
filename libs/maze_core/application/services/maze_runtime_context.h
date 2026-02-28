#ifndef MAZE_APPLICATION_SERVICES_MAZE_RUNTIME_CONTEXT_H
#define MAZE_APPLICATION_SERVICES_MAZE_RUNTIME_CONTEXT_H

#include <utility>

#include "config/config.h"
#include "domain/grid_topology.h"
#include "domain/maze_generation.h"
#include "domain/maze_grid.h"
#include "domain/maze_solver.h"

namespace MazeApplication {

struct MazeRuntimeContext {
  const MazeDomain::IGridTopology* topology_ = nullptr;
  MazeDomain::MazeGrid* maze_grid_ = nullptr;
  std::pair<int, int> start_node_ = {0, 0};
  std::pair<int, int> end_node_ = {0, 0};
  MazeDomain::MazeAlgorithmType generation_algorithm_ =
      MazeDomain::MazeAlgorithmType::DFS;
  MazeSolverDomain::SolverAlgorithmType solver_algorithm_ =
      MazeSolverDomain::SolverAlgorithmType::BFS;
  const Config::AppConfig* app_config_ = nullptr;

  [[nodiscard]] auto IsValid() const -> bool;
};

auto BuildRuntimeContext2D(MazeDomain::MazeGrid& maze_grid,
                           const MazeDomain::IGridTopology& topology,
                           MazeDomain::MazeAlgorithmType generation_algorithm,
                           MazeSolverDomain::SolverAlgorithmType solver_algorithm,
                           const Config::AppConfig& app_config)
    -> MazeRuntimeContext;

}  // namespace MazeApplication

#endif  // MAZE_APPLICATION_SERVICES_MAZE_RUNTIME_CONTEXT_H
