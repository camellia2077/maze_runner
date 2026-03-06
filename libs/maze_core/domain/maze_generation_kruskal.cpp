#include "domain/maze_generation_algorithms_internal.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "domain/grid_topology.h"
#include "domain/maze_generation_grid_ops.h"

namespace MazeDomain::MazeGenerationDetail {
namespace {

struct WallEdge {
  int row1;
  int col1;
  int row2;
  int col2;
  Direction dir_from_r1;
};

class DSU {
 public:
  explicit DSU(int n) : parent_(n), rank_(n, 0) {
    std::iota(parent_.begin(), parent_.end(), 0);
  }

  auto Find(int index) -> int {
    if (parent_[index] == index) {
      return index;
    }
    return parent_[index] = Find(parent_[index]);
  }

  void Unite(int left_index, int right_index) {
    int root_i = Find(left_index);
    int root_j = Find(right_index);
    if (root_i != root_j) {
      if (rank_[root_i] < rank_[root_j]) {
        std::swap(root_i, root_j);
      }
      parent_[root_j] = root_i;
      if (rank_[root_i] == rank_[root_j]) {
        rank_[root_i]++;
      }
    }
  }

 private:
  std::vector<int> parent_;
  std::vector<int> rank_;
};

}  // namespace

void GenerateMazeKruskal(MazeGrid& maze, int /*start_row*/, int /*start_col*/,
                         int width, int height) {
  FillAllWalls(maze, width, height);

  const GridTopology2D topology(GridSize2D{.height = height, .width = width});
  std::vector<WallEdge> all_walls;
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      for (const auto& neighbor : AdjacentWithDirection(topology, row, col)) {
        const Direction dir = neighbor.first;
        if (dir != Direction::Right && dir != Direction::Down) {
          continue;
        }
        all_walls.push_back(
            {row, col, neighbor.second.first, neighbor.second.second, dir});
      }
    }
  }

  std::random_device random_device;
  std::mt19937 engine(random_device());
  std::shuffle(all_walls.begin(), all_walls.end(), engine);

  DSU dsu(width * height);
  int num_edges_added = 0;
  const int total_cells = width * height;

  for (const auto& wall_edge : all_walls) {
    if (num_edges_added >= total_cells - 1 && total_cells > 0) {
      break;
    }

    const auto cell1_index =
        topology.IndexFor(Position2D{.row = wall_edge.row1, .col = wall_edge.col1});
    const auto cell2_index =
        topology.IndexFor(Position2D{.row = wall_edge.row2, .col = wall_edge.col2});
    if (!cell1_index.has_value() || !cell2_index.has_value()) {
      continue;
    }

    if (dsu.Find(*cell1_index) != dsu.Find(*cell2_index)) {
      Carve(maze, wall_edge.row1, wall_edge.col1, wall_edge.dir_from_r1);
      dsu.Unite(*cell1_index, *cell2_index);
      num_edges_added++;
    }
  }
}

}  // namespace MazeDomain::MazeGenerationDetail
