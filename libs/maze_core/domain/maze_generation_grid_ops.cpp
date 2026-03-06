#include "domain/maze_generation_grid_ops.h"

#include <optional>

namespace MazeDomain::MazeGenerationDetail {
namespace {

auto TryDirectionFromTo(CellPosition from, CellPosition to)
    -> std::optional<Direction> {
  const int row_delta = to.first - from.first;
  const int col_delta = to.second - from.second;
  if (row_delta == -1 && col_delta == 0) {
    return Direction::Up;
  }
  if (row_delta == 0 && col_delta == 1) {
    return Direction::Right;
  }
  if (row_delta == 1 && col_delta == 0) {
    return Direction::Down;
  }
  if (row_delta == 0 && col_delta == -1) {
    return Direction::Left;
  }
  return std::nullopt;
}

}  // namespace

auto DirectionIndex(Direction dir) -> int {
  return static_cast<int>(dir);
}

auto IsWithinBounds(int row, int col, GridSize grid_size) -> bool {
  return row >= 0 && row < grid_size.height && col >= 0 && col < grid_size.width;
}

void Carve(Grid& grid, int row, int col, Direction dir) {
  const DirectionInfo info = kDirectionTable[DirectionIndex(dir)];
  const int next_row = row + info.dr;
  const int next_col = col + info.dc;
  grid[row][col].walls[DirectionIndex(dir)] = false;
  grid[next_row][next_col].walls[DirectionIndex(info.opposite)] = false;
}

auto AdjacentWithDirection(const GridTopology2D& topology, int row, int col)
    -> std::vector<NeighborWithDirection> {
  std::vector<NeighborWithDirection> result;
  const auto current_cell_id = topology.IndexFor(Position2D{.row = row, .col = col});
  if (!current_cell_id.has_value()) {
    return result;
  }

  const std::vector<CellId> neighbors = topology.AdjacentCells(*current_cell_id);
  result.reserve(neighbors.size());
  for (const CellId neighbor_id : neighbors) {
    const auto neighbor_pos = topology.PositionFor(neighbor_id);
    if (!neighbor_pos.has_value()) {
      continue;
    }
    const CellPosition next_cell = {neighbor_pos->row, neighbor_pos->col};
    const auto dir = TryDirectionFromTo(CellPosition{row, col}, next_cell);
    if (!dir.has_value()) {
      continue;
    }
    result.emplace_back(*dir, next_cell);
  }
  return result;
}

auto CreateVisitedGrid(GridSize grid_size) -> std::vector<std::vector<bool>> {
  const auto height_size = static_cast<size_t>(grid_size.height);
  const auto width_size = static_cast<size_t>(grid_size.width);
  return std::vector<std::vector<bool>>(height_size,
                                        std::vector<bool>(width_size, false));
}

void FillAllWalls(Grid& maze_grid, int width, int height) {
  for (int row_index = 0; row_index < height; ++row_index) {
    for (int col_index = 0; col_index < width; ++col_index) {
      for (bool& wall : maze_grid[row_index][col_index].walls) {
        wall = true;
      }
    }
  }
}

}  // namespace MazeDomain::MazeGenerationDetail
