#include "application/services/maze_generation.h"

#include <iostream>

#include "application/services/maze_runtime_context.h"

namespace MazeGeneration {

void generate_maze_structure(MazeGrid& maze_grid_to_populate, int start_r,
                             int start_c, int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type) {
  if (algorithm_type == MazeAlgorithmType::DFS ||
      algorithm_type == MazeAlgorithmType::PRIMS ||
      algorithm_type == MazeAlgorithmType::GROWING_TREE) {
    if (start_r < 0 || start_r >= grid_height || start_c < 0 ||
        start_c >= grid_width) {
      std::cerr << "Warning: Maze generation start coordinates (" << start_r
                << "," << start_c << ") out of bounds for grid (" << grid_height
                << "x" << grid_width
                << ") for DFS/Prims/Growing Tree. Defaulting to (0,0)."
                << std::endl;
      start_r = 0;
      start_c = 0;
    }
  }

  std::string name = MazeDomain::algorithm_name(algorithm_type);
  if (name.empty()) {
    std::cerr << "Error: Unknown maze generation algorithm specified. "
                 "Defaulting to DFS."
              << std::endl;
  } else {
    std::cout << "Using " << name << " for maze generation." << std::endl;
  }

  MazeDomain::generate_maze_structure(maze_grid_to_populate, start_r, start_c,
                                      grid_width, grid_height, algorithm_type);
}

void generate_maze_structure(
    const MazeApplication::MazeRuntimeContext& runtime_context) {
  if (!runtime_context.IsValid()) {
    std::cerr << "Error: Invalid runtime context for maze generation."
              << std::endl;
    return;
  }

  const auto* topology_2d =
      dynamic_cast<const MazeDomain::GridTopology2D*>(runtime_context.topology_);
  if (topology_2d == nullptr) {
    std::cerr << "Error: Runtime topology is not 2D for maze generation."
              << std::endl;
    return;
  }

  const MazeDomain::GridSize2D grid_size = topology_2d->Size();
  MazeGeneration::generate_maze_structure(
      *runtime_context.maze_grid_, runtime_context.start_node_.first,
      runtime_context.start_node_.second, grid_size.width, grid_size.height,
      runtime_context.generation_algorithm_);
}

auto algorithm_name(MazeAlgorithmType algorithm_type) -> std::string {
  return MazeDomain::algorithm_name(algorithm_type);
}

auto try_parse_algorithm(std::string_view name, MazeAlgorithmType& out_type)
    -> bool {
  return MazeDomain::try_parse_algorithm(name, out_type);
}

auto supported_algorithms() -> std::vector<std::string> {
  return MazeDomain::supported_algorithms();
}

}  // namespace MazeGeneration
