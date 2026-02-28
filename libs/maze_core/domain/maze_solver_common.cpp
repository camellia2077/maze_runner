#include "domain/maze_solver_common.h"

#include <algorithm>
#include <array>
#include <cstdlib>

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

struct StraightLineTriplet {
  GridPosition first;
  GridPosition second;
  GridPosition third;
};

auto IsStraightLine(StraightLineTriplet line) -> bool {
  if (line.first == kInvalidCell || line.second == kInvalidCell ||
      line.third == kInvalidCell) {
    return false;
  }
  const int kDeltaR1 = line.second.first - line.first.first;
  const int kDeltaC1 = line.second.second - line.first.second;
  const int kDeltaR2 = line.third.first - line.second.first;
  const int kDeltaC2 = line.third.second - line.second.second;
  return kDeltaR1 == kDeltaR2 && kDeltaC1 == kDeltaC2;
}

auto ShouldSkipStraightLineFrame(GridSize grid_size, const ParentGrid& parents,
                                 GridPosition current) -> bool {
  const MazeDomain::CellId kParentId = parents[current.first][current.second];
  if (kParentId == kInvalidCellId) {
    return false;
  }
  const GridPosition kParentCell = ToGridPosition(grid_size, kParentId);
  if (kParentCell == kInvalidCell) {
    return false;
  }
  const MazeDomain::CellId kGrandparentId =
      parents[kParentCell.first][kParentCell.second];
  if (kGrandparentId == kInvalidCellId) {
    return false;
  }
  const GridPosition kGrandparentCell = ToGridPosition(grid_size, kGrandparentId);
  if (kGrandparentCell == kInvalidCell) {
    return false;
  }
  const StraightLineTriplet kLine{
      .first = kGrandparentCell, .second = kParentCell, .third = current};
  return IsStraightLine(kLine);
}

void AppendSolutionPath(GridSize grid_size, const PathEndpoints& endpoints,
                        const ParentGrid& parents,
                        StateGrid& visual_states, SearchResult& result) {
  std::vector<GridPosition> path;
  GridPosition path_node = endpoints.end;
  while (path_node != kInvalidCell) {
    path.push_back(path_node);
    visual_states[path_node.first][path_node.second] =
        SolverCellState::SOLUTION;
    if (path_node == endpoints.start) {
      break;
    }
    const MazeDomain::CellId kParentId =
        parents[path_node.first][path_node.second];
    path_node = ToGridPosition(grid_size, kParentId);
  }
  std::ranges::reverse(path);
  result.path_ = path;
  PushFrame(result, visual_states, path);
}

}  // namespace

auto GetGridSize(const MazeGrid& maze_grid) -> std::optional<GridSize> {
  const int kHeight = static_cast<int>(maze_grid.size());
  if (kHeight <= 0) {
    return std::nullopt;
  }
  const int kWidth = static_cast<int>(maze_grid[0].size());
  if (kWidth <= 0) {
    return std::nullopt;
  }
  return GridSize{.height = kHeight, .width = kWidth};
}

auto IsValidPosition(GridPosition pos, GridSize grid_size) -> bool {
  return pos.first >= 0 && pos.first < grid_size.height && pos.second >= 0 &&
         pos.second < grid_size.width;
}

auto CreateBoolGrid(GridSize grid_size, bool initial) -> BoolGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<bool>(kWidthSize, initial)};
}

auto CreateIntGrid(GridSize grid_size, int initial) -> IntGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<int>(kWidthSize, initial)};
}

auto CreateStateGrid(GridSize grid_size, SolverCellState initial) -> StateGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<SolverCellState>(kWidthSize, initial)};
}

auto CreateParentGrid(GridSize grid_size, MazeDomain::CellId initial)
    -> ParentGrid {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return {kHeightSize, std::vector<MazeDomain::CellId>(kWidthSize, initial)};
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
  const int kTotalCellCount = grid_size.height * grid_size.width;
  if (cell_id < 0 || cell_id >= kTotalCellCount) {
    return kInvalidCell;
  }
  return {cell_id / grid_size.width, cell_id % grid_size.width};
}

void PushFrame(SearchResult& result,
               const StateGrid& visual_states,
               const std::vector<GridPosition>& current_path) {
  SearchFrame frame;
  frame.visual_states_ = visual_states;
  frame.current_path_ = current_path;
  result.frames_.push_back(std::move(frame));
}

auto ShouldSaveFrameForCurrent(const ParentGrid& parents, GridPosition current,
                               const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.end) {
    return true;
  }
  const GridSize kGridSize{.height = static_cast<int>(parents.size()),
                           .width = parents.empty()
                                        ? 0
                                        : static_cast<int>(parents.front().size())};
  return !ShouldSkipStraightLineFrame(kGridSize, parents, current);
}

auto ShouldSaveBacktrackFrame(const ParentGrid& parents, GridPosition current,
                              const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.start || current == endpoints.end) {
    return true;
  }
  const GridSize kGridSize{.height = static_cast<int>(parents.size()),
                           .width = parents.empty()
                                        ? 0
                                        : static_cast<int>(parents.front().size())};
  return !ShouldSkipStraightLineFrame(kGridSize, parents, current);
}

auto CreateTrivialResult(GridSize grid_size, GridPosition node) -> SearchResult {
  SearchResult result;
  auto visual_states = CreateStateGrid(grid_size, SolverCellState::NONE);
  auto visited = CreateBoolGrid(grid_size, false);
  visited[node.first][node.second] = true;
  visual_states[node.first][node.second] = SolverCellState::SOLUTION;
  result.path_.push_back(node);
  PushFrame(result, visual_states, result.path_);
  result.explored_ = std::move(visited);
  result.found_ = true;
  return result;
}

void FinalizeSearchResult(bool found, const PathEndpoints& endpoints,
                          const ParentGrid& parents,
                          StateGrid& visual_states, BoolGrid&& visited,
                          SearchResult& result) {
  const GridSize kGridSize{.height = static_cast<int>(parents.size()),
                           .width = parents.empty()
                                        ? 0
                                        : static_cast<int>(parents.front().size())};
  if (found) {
    AppendSolutionPath(kGridSize, endpoints, parents, visual_states, result);
  } else {
    PushFrame(result, visual_states, {});
  }

  result.found_ = found;
  result.explored_ = std::move(visited);
}

auto ManhattanDistance(PositionPair positions) -> int {
  return std::abs(positions.first.first - positions.second.first) +
         std::abs(positions.first.second - positions.second.second);
}

auto ReachableNeighbors(const MazeGrid& maze_grid, GridSize grid_size,
                        GridPosition current, NeighborOrder order)
    -> std::vector<GridPosition> {
  std::vector<GridPosition> neighbors;
  neighbors.reserve(kWallCount);

  const auto& deltas = DeltaTableFor(order);
  for (const NeighborDelta& delta : deltas) {
    const int kNextRow = current.first + delta.row_delta;
    const int kNextCol = current.second + delta.col_delta;
    const bool kBlockedByWall =
        maze_grid[current.first][current.second].walls[delta.wall_index];

    if (kNextRow >= 0 && kNextRow < grid_size.height && kNextCol >= 0 &&
        kNextCol < grid_size.width && !kBlockedByWall) {
      neighbors.emplace_back(kNextRow, kNextCol);
    }
  }

  return neighbors;
}

}  // namespace MazeSolverDomain::detail
