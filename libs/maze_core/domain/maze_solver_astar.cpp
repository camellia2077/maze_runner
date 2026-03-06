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
                GridPosition end_node, ISearchEventSink* event_sink,
                const SolveOptions& options) -> SearchResult {
  auto execution_state = CreateExecutionState(event_sink, options);
  EmitRunStarted(execution_state);

  SearchResult result;
  const auto kGridSize = GetGridSize(maze_grid);
  if (!kGridSize.has_value()) {
    EmitRunFailed(execution_state, "Invalid maze grid.");
    return result;
  }
  if (!IsValidPosition(start_node, *kGridSize) ||
      !IsValidPosition(end_node, *kGridSize)) {
    EmitRunFailed(execution_state, "Start or end node out of bounds.");
    return result;
  }

  if (start_node == end_node) {
    return CreateTrivialResult(*kGridSize, start_node, execution_state);
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
  EmitProgress(execution_state, visual_states, {}, true);

  bool found = false;
  while (!frontier.empty() && !found) {
    if (ShouldCancel(execution_state)) {
      result.cancelled_ = true;
      break;
    }

    const AStarNode kCurrentNode = frontier.top();
    frontier.pop();

    const GridPosition kCurrent = kCurrentNode.pos;
    if (visited[kCurrent.first][kCurrent.second]) {
      continue;
    }

    const bool kShouldEmitProgress =
        ShouldEmitProgressForCurrent(parents, kCurrent, kEndpoints);

    visited[kCurrent.first][kCurrent.second] = true;
    visual_states[kCurrent.first][kCurrent.second] =
        SolverCellState::CURRENT_PROC;
    if (kShouldEmitProgress) {
      EmitProgress(execution_state, visual_states, {}, false);
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

    if (kShouldEmitProgress) {
      EmitProgress(execution_state, visual_states, {}, false);
    }
  }

  if (result.cancelled_) {
    result.explored_ = std::move(visited);
    EmitRunCancelled(execution_state, "Cancelled by sink.");
    return result;
  }

  FinalizeSearchResult(found, kEndpoints, parents, visual_states,
                       std::move(visited), execution_state, result);
  return result;
}

}  // namespace MazeSolverDomain::detail
