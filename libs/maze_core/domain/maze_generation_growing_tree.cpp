#include "domain/maze_generation_algorithms_internal.h"

#include <random>
#include <vector>

#include "domain/grid_topology.h"
#include "domain/maze_generation_grid_ops.h"

namespace MazeDomain::MazeGenerationDetail {

void GenerateMazeGrowingTree(MazeGrid& maze, int start_row, int start_col,
                             int width, int height) {
  const GridSize grid_size{.width = width, .height = height};
  const GridTopology2D topology(GridSize2D{.height = height, .width = width});
  auto visited = CreateVisitedGrid(grid_size);
  std::vector<CellPosition> active_cells;
  active_cells.push_back({start_row, start_col});
  visited[start_row][start_col] = true;

  std::random_device random_device;
  std::mt19937 engine(random_device());

  while (!active_cells.empty()) {
    std::uniform_int_distribution<int> index_dist(
        0, static_cast<int>(active_cells.size()) - 1);
    const int cell_index = index_dist(engine);
    const CellPosition cell = active_cells[cell_index];

    std::vector<NeighborWithDirection> neighbors;
    for (const auto& neighbor :
         AdjacentWithDirection(topology, cell.first, cell.second)) {
      const int next_row = neighbor.second.first;
      const int next_col = neighbor.second.second;
      if (!visited[next_row][next_col]) {
        neighbors.push_back(neighbor);
      }
    }

    if (neighbors.empty()) {
      active_cells[cell_index] = active_cells.back();
      active_cells.pop_back();
      continue;
    }

    std::uniform_int_distribution<int> dir_dist(
        0, static_cast<int>(neighbors.size()) - 1);
    const auto next = neighbors[dir_dist(engine)];
    const Direction dir = next.first;
    const int next_row = next.second.first;
    const int next_col = next.second.second;
    Carve(maze, cell.first, cell.second, dir);
    visited[next_row][next_col] = true;
    active_cells.push_back({next_row, next_col});
  }
}

}  // namespace MazeDomain::MazeGenerationDetail
