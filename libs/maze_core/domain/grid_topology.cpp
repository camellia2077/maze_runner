#include "domain/grid_topology.h"

namespace MazeDomain {

namespace {

auto IsValidSize(GridSize2D grid_size) -> bool {
  return grid_size.height > 0 && grid_size.width > 0;
}

}  // namespace

GridTopology2D::GridTopology2D(GridSize2D grid_size) : grid_size_(grid_size) {}

auto GridTopology2D::Dimension() const -> MazeDimension {
  return MazeDimension::D2;
}

auto GridTopology2D::CellCount() const -> int {
  if (!IsValidSize(grid_size_)) {
    return 0;
  }
  return grid_size_.height * grid_size_.width;
}

auto GridTopology2D::IsValidCell(CellId cell_id) const -> bool {
  return cell_id >= 0 && cell_id < CellCount();
}

auto GridTopology2D::AdjacentCells(CellId cell_id) const -> std::vector<CellId> {
  std::vector<CellId> neighbors;
  neighbors.reserve(kNeighborCount2D);
  const auto current_pos = PositionFor(cell_id);
  if (!current_pos.has_value()) {
    return neighbors;
  }

  const Position2D up_pos{.row = current_pos->row - kRowMajorBase,
                          .col = current_pos->col};
  const Position2D right_pos{.row = current_pos->row,
                             .col = current_pos->col + kRowMajorBase};
  const Position2D down_pos{.row = current_pos->row + kRowMajorBase,
                            .col = current_pos->col};
  const Position2D left_pos{.row = current_pos->row,
                            .col = current_pos->col - kRowMajorBase};

  for (const Position2D next_pos : {up_pos, right_pos, down_pos, left_pos}) {
    const auto next_index = IndexFor(next_pos);
    if (next_index.has_value()) {
      neighbors.push_back(*next_index);
    }
  }

  return neighbors;
}

auto GridTopology2D::IsValidPosition(Position2D pos) const -> bool {
  return pos.row >= 0 && pos.row < grid_size_.height && pos.col >= 0 &&
         pos.col < grid_size_.width;
}

auto GridTopology2D::IndexFor(Position2D pos) const -> std::optional<CellId> {
  if (!IsValidPosition(pos)) {
    return std::nullopt;
  }
  return (pos.row * grid_size_.width) + pos.col;
}

auto GridTopology2D::PositionFor(CellId cell_id) const
    -> std::optional<Position2D> {
  if (!IsValidCell(cell_id) || grid_size_.width <= 0) {
    return std::nullopt;
  }
  return Position2D{.row = cell_id / grid_size_.width,
                    .col = cell_id % grid_size_.width};
}

auto GridTopology2D::Size() const -> GridSize2D {
  return grid_size_;
}

}  // namespace MazeDomain
