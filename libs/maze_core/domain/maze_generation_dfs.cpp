#include "domain/maze_generation_algorithms_internal.h"

#include <algorithm>
#include <random>
#include <vector>

#include "domain/grid_topology.h"
#include "domain/maze_generation_grid_ops.h"

namespace MazeDomain::MazeGenerationDetail {
namespace {

void GenerateMazeRecursiveInternal(int row, int col, Grid& current_maze_data,
                                   std::vector<std::vector<bool>>& visited,
                                   const GridTopology2D& topology) {
  visited[row][col] = true;
  auto neighbors = AdjacentWithDirection(topology, row, col);

  std::random_device random_device;
  std::mt19937 engine(random_device());
  std::shuffle(neighbors.begin(), neighbors.end(), engine);

  for (const auto& neighbor : neighbors) {
    const Direction dir = neighbor.first;
    const int next_row = neighbor.second.first;
    const int next_col = neighbor.second.second;
    if (!visited[next_row][next_col]) {
      Carve(current_maze_data, row, col, dir);
      GenerateMazeRecursiveInternal(next_row, next_col, current_maze_data,
                                    visited, topology);
    }
  }
}

}  // namespace

void GenerateMazeDfs(MazeGrid& maze, int start_row, int start_col, int width,
                     int height) {
  const GridSize grid_size{.width = width, .height = height};
  const GridTopology2D topology(GridSize2D{.height = height, .width = width});
  auto visited = CreateVisitedGrid(grid_size);
  GenerateMazeRecursiveInternal(start_row, start_col, maze, visited, topology);
}

}  // namespace MazeDomain::MazeGenerationDetail
