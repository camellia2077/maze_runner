#include "domain/maze_generation_algorithms_internal.h"

#include <random>
#include <vector>

#include "domain/grid_topology.h"
#include "domain/maze_generation_grid_ops.h"

namespace MazeDomain::MazeGenerationDetail {
namespace {

struct FrontierEdge {
  int row1;
  int col1;
  int row2;
  int col2;
  Direction dir_from_r1;
};

}  // namespace

void GenerateMazePrims(MazeGrid& maze, int start_row, int start_col, int width,
                       int height) {
  const GridSize grid_size{.width = width, .height = height};
  auto visited = CreateVisitedGrid(grid_size);
  FillAllWalls(maze, width, height);

  std::random_device random_device;
  std::mt19937 engine(random_device());

  const GridTopology2D topology(GridSize2D{.height = height, .width = width});
  visited[start_row][start_col] = true;
  std::vector<FrontierEdge> frontier_walls;

  for (const auto& neighbor :
       AdjacentWithDirection(topology, start_row, start_col)) {
    frontier_walls.push_back({start_row, start_col, neighbor.second.first,
                              neighbor.second.second, neighbor.first});
  }

  while (!frontier_walls.empty()) {
    std::uniform_int_distribution<> distrib(
        0, static_cast<int>(frontier_walls.size()) - 1);
    const int random_index = distrib(engine);
    const FrontierEdge frontier_edge = frontier_walls[random_index];

    frontier_walls[random_index] = frontier_walls.back();
    frontier_walls.pop_back();

    if (!visited[frontier_edge.row2][frontier_edge.col2]) {
      Carve(maze, frontier_edge.row1, frontier_edge.col1,
            frontier_edge.dir_from_r1);
      visited[frontier_edge.row2][frontier_edge.col2] = true;

      for (const auto& neighbor :
           AdjacentWithDirection(topology, frontier_edge.row2,
                                 frontier_edge.col2)) {
        const int next_row = neighbor.second.first;
        const int next_col = neighbor.second.second;
        if (!visited[next_row][next_col]) {
          frontier_walls.push_back({frontier_edge.row2, frontier_edge.col2,
                                    next_row, next_col, neighbor.first});
        }
      }
    }
  }
}

}  // namespace MazeDomain::MazeGenerationDetail
