#include "domain/maze_solver_algorithms.h"

#include <limits>
#include <queue>

namespace MazeSolverDomain::detail {

namespace {

void EnqueueAStarNeighbors(
    const MazeGrid& maze_grid, GridSize grid_size,
    const SearchTargets& targets,
    const BoolGrid& visited, IntGrid& g_scores, ParentGrid& parents,
    std::priority_queue<AStarNode, std::vector<AStarNode>, AStarNodeCompare>&
        frontier,
    StateGrid& visual_states) {
  const auto neighbors = ReachableNeighbors(
      maze_grid, grid_size, targets.current, NeighborOrder::VerticalFirst);
  for (const GridPosition& next : neighbors) {
    if (!visited[next.first][next.second]) {
      const int kTentativeG =
          g_scores[targets.current.first][targets.current.second] + 1;
      if (kTentativeG < g_scores[next.first][next.second]) {
        g_scores[next.first][next.second] = kTentativeG;
        parents[next.first][next.second] =
            ToCellId(grid_size, targets.current);
        const GridPosition kNextPos = next;
        const PositionPair kNextToEnd{
            .first = kNextPos, .second = targets.end};
        const int kFScore = kTentativeG + ManhattanDistance(kNextToEnd);
        frontier.push({kFScore, kTentativeG, next});
        visual_states[next.first][next.second] = SolverCellState::FRONTIER;
      }
    }
  }
}

}  // namespace

auto SolveAStar(const MazeGrid& maze_grid, GridPosition start_node,
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

  const int kMaxCost = std::numeric_limits<int>::max() / kMaxCostDivisor;
  auto g_scores = CreateIntGrid(*kGridSize, kMaxCost);
  std::priority_queue<AStarNode, std::vector<AStarNode>, AStarNodeCompare>
      frontier;

  g_scores[start_node.first][start_node.second] = 0;
  const PositionPair kStartEnd{.first = start_node, .second = end_node};
  frontier.push({ManhattanDistance(kStartEnd), 0, start_node});
  visual_states[start_node.first][start_node.second] =
      SolverCellState::FRONTIER;
  PushFrame(result, visual_states, {});

  bool found = false;
  while (!frontier.empty() && !found) {
    const AStarNode kCurrentNode = frontier.top();
    frontier.pop();

    const GridPosition kCurrent = kCurrentNode.pos;
    if (visited[kCurrent.first][kCurrent.second]) {
      continue;
    }

    const bool kShouldSaveFrame =
        ShouldSaveFrameForCurrent(parents, kCurrent, kEndpoints);

    visited[kCurrent.first][kCurrent.second] = true;
    visual_states[kCurrent.first][kCurrent.second] =
        SolverCellState::CURRENT_PROC;
    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }

    if (kCurrent == end_node) {
      found = true;
      break;
    }

    const SearchTargets kTargets{.current = kCurrent, .end = end_node};
    EnqueueAStarNeighbors(maze_grid, *kGridSize, kTargets, visited, g_scores,
                          parents, frontier, visual_states);

    if (kCurrent != end_node || !found) {
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::VISITED_PROC;
    }

    if (kShouldSaveFrame) {
      PushFrame(result, visual_states, {});
    }
  }

  FinalizeSearchResult(found, kEndpoints, parents, visual_states,
                       std::move(visited), result);
  return result;
}

}  // namespace MazeSolverDomain::detail
