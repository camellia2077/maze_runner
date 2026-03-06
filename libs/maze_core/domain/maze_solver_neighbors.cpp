#include "domain/maze_solver_neighbors.h"

#include <array>

namespace MazeSolverDomain::detail {
namespace {

inline constexpr int kWallTop = 0;
inline constexpr int kWallRight = 1;
inline constexpr int kWallBottom = 2;
inline constexpr int kWallLeft = 3;

struct NeighborDelta {
  int row_delta;
  int col_delta;
  int wall_index;
};

constexpr std::array<NeighborDelta, kWallCount> kVerticalFirstDeltas = {{
    {.row_delta = -1, .col_delta = 0, .wall_index = kWallTop},
    {.row_delta = 1, .col_delta = 0, .wall_index = kWallBottom},
    {.row_delta = 0, .col_delta = -1, .wall_index = kWallLeft},
    {.row_delta = 0, .col_delta = 1, .wall_index = kWallRight},
}};

constexpr std::array<NeighborDelta, kWallCount> kClockwiseDeltas = {{
    {.row_delta = -1, .col_delta = 0, .wall_index = kWallTop},
    {.row_delta = 0, .col_delta = 1, .wall_index = kWallRight},
    {.row_delta = 1, .col_delta = 0, .wall_index = kWallBottom},
    {.row_delta = 0, .col_delta = -1, .wall_index = kWallLeft},
}};

auto DeltaTableFor(NeighborOrder order)
    -> const std::array<NeighborDelta, kWallCount>& {
  if (order == NeighborOrder::Clockwise) {
    return kClockwiseDeltas;
  }
  return kVerticalFirstDeltas;
}

}  // namespace

auto ReachableNeighbors(const MazeGrid& maze_grid, GridSize grid_size,
                        GridPosition current, NeighborOrder order)
    -> std::vector<GridPosition> {
  std::vector<GridPosition> neighbors;
  neighbors.reserve(kWallCount);

  const auto& deltas = DeltaTableFor(order);
  for (const NeighborDelta& delta : deltas) {
    const int next_row = current.first + delta.row_delta;
    const int next_col = current.second + delta.col_delta;
    const bool blocked_by_wall =
        maze_grid[current.first][current.second].walls[delta.wall_index];

    if (next_row >= 0 && next_row < grid_size.height && next_col >= 0 &&
        next_col < grid_size.width && !blocked_by_wall) {
      neighbors.emplace_back(next_row, next_col);
    }
  }

  return neighbors;
}

}  // namespace MazeSolverDomain::detail
