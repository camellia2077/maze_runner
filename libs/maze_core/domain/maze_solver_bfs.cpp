#include "domain/maze_solver_algorithms.h"

#include <queue>

namespace MazeSolverDomain::detail {

namespace {

void EnqueueBfsNeighbors(const MazeGrid& maze_grid, GridSize grid_size,
                         GridPosition current,
                         BoolGrid& visited, ParentGrid& parents,
                         std::queue<GridPosition>& frontier,
                         StateGrid& visual_states) {
  const auto neighbors = ReachableNeighbors(maze_grid, grid_size, current,
                                            NeighborOrder::VerticalFirst);
  for (const GridPosition& next : neighbors) {
    if (!visited[next.first][next.second]) {
      visited[next.first][next.second] = true;
      parents[next.first][next.second] = ToCellId(grid_size, current);
      frontier.emplace(next.first, next.second);
      visual_states[next.first][next.second] = SolverCellState::FRONTIER;
    }
  }
}

}  // namespace

auto SolveBfs(const MazeGrid& maze_grid, GridPosition start_node,
              GridPosition end_node) -> SearchResult {
  SearchResult result;
  const auto kGridSize = GetGridSize(maze_grid);
  if (!kGridSize.has_value()) {
    return result;
  }
  if (!IsValidPosition(start_node, *kGridSize) ||
      !IsValidPosition(end_node, *kGridSize)) {
    return result;
  }

  if (start_node == end_node) {
    return CreateTrivialResult(*kGridSize, start_node);
  }

  const PathEndpoints kEndpoints{.start = start_node, .end = end_node};
  auto visual_states = CreateStateGrid(*kGridSize, SolverCellState::NONE);
  auto visited = CreateBoolGrid(*kGridSize, false);
  auto parents = CreateParentGrid(*kGridSize, kInvalidCellId);

  std::queue<GridPosition> frontier;
  frontier.push(start_node);
  visited[start_node.first][start_node.second] = true;
  visual_states[start_node.first][start_node.second] =
      SolverCellState::FRONTIER;
  PushFrame(result, visual_states, {});

  bool found = false;
  while (!frontier.empty() && !found) {
    const GridPosition kCurrent = frontier.front();
    frontier.pop();

    const bool kShouldSaveFrame =
        ShouldSaveFrameForCurrent(parents, kCurrent, kEndpoints);

    visual_states[kCurrent.first][kCurrent.second] =
        SolverCellState::CURRENT_PROC;
    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }

    if (kCurrent == end_node) {
      found = true;
    }

    if (!found) {
      EnqueueBfsNeighbors(maze_grid, *kGridSize, kCurrent, visited,
                          parents, frontier, visual_states);
    }

    if (kCurrent != end_node || !found) {
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::VISITED_PROC;
    }

    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }
    if (found && kCurrent == end_node) {
      break;
    }
  }

  FinalizeSearchResult(found, kEndpoints, parents, visual_states,
                       std::move(visited), result);
  return result;
}

}  // namespace MazeSolverDomain::detail
