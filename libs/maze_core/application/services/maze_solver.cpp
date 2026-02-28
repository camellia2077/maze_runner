#include "application/services/maze_solver.h"

#include <iostream>

#include "application/services/maze_runtime_context.h"

namespace {

using GridPosition = MazeSolverDomain::GridPosition;

auto IsValidPosition(GridPosition pos, int height, int width) -> bool {
  return pos.first >= 0 && pos.first < height && pos.second >= 0 &&
         pos.second < width;
}

auto SolveWithResolvedInputs(const MazeDomain::MazeGrid& maze_data,
                             MazeSolver::SolverAlgorithmType algorithm_type,
                             GridPosition start_node, GridPosition end_node,
                             int grid_height, int grid_width)
    -> MazeSolver::SearchResult {
  const std::string kSolverName =
      MazeSolverDomain::AlgorithmName(algorithm_type);
  const std::string kDisplayName = kSolverName.empty() ? "Solver" : kSolverName;

  if (grid_width <= 0 || grid_height <= 0) {
    std::cout << kDisplayName << ": Invalid maze dimensions. Aborting."
              << std::endl;
    return {};
  }

  const auto kHeightSize = static_cast<size_t>(grid_height);
  const auto kWidthSize = static_cast<size_t>(grid_width);
  if (maze_data.size() != kHeightSize) {
    std::cerr << kDisplayName
              << ": Maze grid height does not match config. Aborting."
              << std::endl;
    return {};
  }
  if (!maze_data.empty() && maze_data.front().size() != kWidthSize) {
    std::cerr << kDisplayName
              << ": Maze grid width does not match config. Aborting."
              << std::endl;
    return {};
  }

  if (!IsValidPosition(start_node, grid_height, grid_width)) {
    std::cerr << kDisplayName << ": Start node is out of bounds. Aborting."
              << std::endl;
    return {};
  }
  if (!IsValidPosition(end_node, grid_height, grid_width)) {
    std::cerr << kDisplayName << ": End node is out of bounds. Aborting."
              << std::endl;
    return {};
  }

  if (start_node == end_node) {
    std::cout << kDisplayName
              << ": Start and End nodes are the same. Path is trivial."
              << std::endl;
  }

  MazeSolver::SearchResult result =
      MazeSolverDomain::Solve(maze_data, start_node, end_node, algorithm_type);

  if (result.found_) {
    std::cout << kDisplayName << ": Path found. Length: " << result.path_.size()
              << std::endl;
  } else {
    std::cout << kDisplayName << ": Path not found." << std::endl;
  }

  return result;
}

}  // namespace

namespace MazeSolver {

auto AlgorithmName(SolverAlgorithmType algorithm_type) -> std::string {
  return MazeSolverDomain::AlgorithmName(algorithm_type);
}

auto TryParseAlgorithm(std::string_view name, SolverAlgorithmType& out_type)
    -> bool {
  return MazeSolverDomain::TryParseAlgorithm(name, out_type);
}

auto SupportedAlgorithms() -> std::vector<std::string> {
  return MazeSolverDomain::supported_algorithms();
}

auto Solve(const MazeDomain::MazeGrid& maze_data,
           SolverAlgorithmType algorithm_type, const Config::AppConfig& config)
    -> SearchResult {
  const auto& maze = config.maze;
  return SolveWithResolvedInputs(maze_data, algorithm_type, maze.start_node,
                                 maze.end_node, maze.height, maze.width);
}

auto Solve(const MazeApplication::MazeRuntimeContext& runtime_context)
    -> SearchResult {
  if (!runtime_context.IsValid()) {
    std::cerr << "Error: Invalid runtime context for maze solver."
              << std::endl;
    return {};
  }

  const auto* topology_2d =
      dynamic_cast<const MazeDomain::GridTopology2D*>(runtime_context.topology_);
  if (topology_2d == nullptr) {
    std::cerr << "Error: Runtime topology is not 2D for maze solver."
              << std::endl;
    return {};
  }

  const MazeDomain::GridSize2D grid_size = topology_2d->Size();
  return SolveWithResolvedInputs(*runtime_context.maze_grid_,
                                 runtime_context.solver_algorithm_,
                                 runtime_context.start_node_,
                                 runtime_context.end_node_, grid_size.height,
                                 grid_size.width);
}

}  // namespace MazeSolver
