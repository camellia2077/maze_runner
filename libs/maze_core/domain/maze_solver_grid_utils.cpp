#include "domain/maze_solver_grid_utils.h"

#include <cstdlib>

namespace MazeSolverDomain::detail {

auto GetGridSize(const MazeGrid& maze_grid) -> std::optional<GridSize> {
  const int height = static_cast<int>(maze_grid.size());
  if (height <= 0) {
    return std::nullopt;
  }
  const int width = static_cast<int>(maze_grid[0].size());
  if (width <= 0) {
    return std::nullopt;
  }
  return GridSize{.height = height, .width = width};
}

auto IsValidPosition(GridPosition pos, GridSize grid_size) -> bool {
  return pos.first >= 0 && pos.first < grid_size.height && pos.second >= 0 &&
         pos.second < grid_size.width;
}

auto CreateBoolGrid(GridSize grid_size, bool initial) -> BoolGrid {
  const auto height_size = static_cast<size_t>(grid_size.height);
  const auto width_size = static_cast<size_t>(grid_size.width);
  return {height_size, std::vector<bool>(width_size, initial)};
}

auto CreateIntGrid(GridSize grid_size, int initial) -> IntGrid {
  const auto height_size = static_cast<size_t>(grid_size.height);
  const auto width_size = static_cast<size_t>(grid_size.width);
  return {height_size, std::vector<int>(width_size, initial)};
}

auto CreateStateGrid(GridSize grid_size, SolverCellState initial) -> StateGrid {
  const auto height_size = static_cast<size_t>(grid_size.height);
  const auto width_size = static_cast<size_t>(grid_size.width);
  return {height_size, std::vector<SolverCellState>(width_size, initial)};
}

auto CreateParentGrid(GridSize grid_size, MazeDomain::CellId initial)
    -> ParentGrid {
  const auto height_size = static_cast<size_t>(grid_size.height);
  const auto width_size = static_cast<size_t>(grid_size.width);
  return {height_size, std::vector<MazeDomain::CellId>(width_size, initial)};
}

auto ToCellId(GridSize grid_size, GridPosition pos) -> MazeDomain::CellId {
  if (!IsValidPosition(pos, grid_size)) {
    return kInvalidCellId;
  }
  return (pos.first * grid_size.width) + pos.second;
}

auto ToGridPosition(GridSize grid_size, MazeDomain::CellId cell_id)
    -> GridPosition {
  if (cell_id == kInvalidCellId || grid_size.width <= 0) {
    return kInvalidCell;
  }
  const int total_cell_count = grid_size.height * grid_size.width;
  if (cell_id < 0 || cell_id >= total_cell_count) {
    return kInvalidCell;
  }
  return {cell_id / grid_size.width, cell_id % grid_size.width};
}

auto ManhattanDistance(PositionPair positions) -> int {
  return std::abs(positions.first.first - positions.second.first) +
         std::abs(positions.first.second - positions.second.second);
}

}  // namespace MazeSolverDomain::detail
