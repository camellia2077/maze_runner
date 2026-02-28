#include "domain/maze_solver_algorithms.h"

#include <queue>

namespace MazeSolverDomain::detail {

namespace {

struct GreedyNode {
  int h_score;
  GridPosition pos;
};

struct GreedyNodeCompare {
  auto operator()(const GreedyNode& left, const GreedyNode& right) const -> bool {
    return left.h_score > right.h_score;
  }
};

void EnqueueGreedyNeighbors(
    const MazeGrid& maze_grid, GridSize grid_size, GridPosition current,
    const SearchTargets& targets,
    const BoolGrid& visited, ParentGrid& parents,
    std::priority_queue<GreedyNode, std::vector<GreedyNode>, GreedyNodeCompare>&
        frontier,
    StateGrid& visual_states) {
  const auto neighbors = ReachableNeighbors(maze_grid, grid_size, current,
                                            NeighborOrder::VerticalFirst);
  for (const GridPosition& next : neighbors) {
    if (!visited[next.first][next.second]) {
      if (parents[next.first][next.second] == kInvalidCellId) {
        parents[next.first][next.second] = ToCellId(grid_size, current);
      }
      const PositionPair kNextToEnd{
          .first = next, .second = targets.end};
      frontier.push({ManhattanDistance(kNextToEnd), next});
      visual_states[next.first][next.second] = SolverCellState::FRONTIER;
    }
  }
}

}  // namespace

auto SolveGreedyBestFirst(const MazeGrid& maze_grid, GridPosition start_node,
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

  std::priority_queue<GreedyNode, std::vector<GreedyNode>, GreedyNodeCompare>
      frontier;

  const PositionPair kStartEnd{.first = start_node, .second = end_node};
  frontier.push({ManhattanDistance(kStartEnd), start_node});
  visual_states[start_node.first][start_node.second] =
      SolverCellState::FRONTIER;
  PushFrame(result, visual_states, {});

  bool found = false;
  while (!frontier.empty() && !found) {
    const GreedyNode kCurrentNode = frontier.top();
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
    EnqueueGreedyNeighbors(maze_grid, *kGridSize, kCurrent, kTargets, visited,
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
