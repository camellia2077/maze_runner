#include "domain/maze_generation.h"

#include <string_view>
#include <vector>

#include "domain/maze_generation_grid_ops.h"

namespace MazeDomain {

void generate_maze_structure(MazeGrid& maze_grid_to_populate, int start_r,
                             int start_c, int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type) {
  if (algorithm_type == MazeAlgorithmType::DFS ||
      algorithm_type == MazeAlgorithmType::PRIMS ||
      algorithm_type == MazeAlgorithmType::GROWING_TREE) {
    const MazeGenerationDetail::GridSize grid_size{
        .width = grid_width, .height = grid_height};
    if (!MazeGenerationDetail::IsWithinBounds(start_r, start_c, grid_size)) {
      start_r = 0;
      start_c = 0;
    }
  }

  MazeGenerationDetail::FillAllWalls(maze_grid_to_populate, grid_width,
                                     grid_height);

  auto generator = MazeGeneratorFactory::instance().get_generator(algorithm_type);
  if (!generator) {
    generator =
        MazeGeneratorFactory::instance().get_generator(MazeAlgorithmType::DFS);
  }
  if (generator) {
    generator(maze_grid_to_populate, start_r, start_c, grid_width, grid_height);
  }
}

auto algorithm_name(MazeAlgorithmType algorithm_type) -> std::string {
  return MazeGeneratorFactory::instance().name_for(algorithm_type);
}

auto try_parse_algorithm(std::string_view name, MazeAlgorithmType& out_type)
    -> bool {
  return MazeGeneratorFactory::instance().try_parse(name, out_type);
}

auto supported_algorithms() -> std::vector<std::string> {
  return MazeGeneratorFactory::instance().names();
}

}  // namespace MazeDomain
