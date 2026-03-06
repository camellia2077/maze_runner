#include "domain/maze_solver_algorithms.h"

#include <stack>

namespace MazeSolverDomain::detail {

namespace {

auto TryPushDfsNeighbor(const MazeGrid& maze_grid, GridSize grid_size,
                        GridPosition current,
                        const BoolGrid& visited, ParentGrid& parents,
                        std::stack<GridPosition>& frontier,
                        StateGrid& visual_states) -> bool {
  const auto neighbors = ReachableNeighbors(maze_grid, grid_size, current,
                                            NeighborOrder::Clockwise);
  for (const GridPosition& next : neighbors) {
    if (!visited[next.first][next.second]) {
      parents[next.first][next.second] = ToCellId(grid_size, current);
      frontier.emplace(next.first, next.second);
      visual_states[next.first][next.second] = SolverCellState::FRONTIER;
      return true;
    }
  }
  return false;
}

}  // namespace

auto SolveDfs(const MazeGrid& maze_grid, GridPosition start_node,
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

  std::stack<GridPosition> frontier;
  frontier.push(start_node);
  visual_states[start_node.first][start_node.second] =
      SolverCellState::FRONTIER;
  EmitProgress(execution_state, visual_states, {}, true);

  bool found = false;
  while (!frontier.empty() && !found) {
    if (ShouldCancel(execution_state)) {
      result.cancelled_ = true;
      break;
    }

    const GridPosition kCurrent = frontier.top();

    if (visual_states[kCurrent.first][kCurrent.second] ==
        SolverCellState::VISITED_PROC) {
      frontier.pop();
      continue;
    }

    bool should_save_frame = true;
    if (!visited[kCurrent.first][kCurrent.second]) {
      visited[kCurrent.first][kCurrent.second] = true;
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::CURRENT_PROC;

      should_save_frame =
          ShouldEmitProgressForCurrent(parents, kCurrent, kEndpoints);

      if (should_save_frame) {
        EmitProgress(execution_state, visual_states, {}, false);
      }
    }

    if (kCurrent == end_node) {
      found = true;
    }

    if (found) {
      break;
    }

    const bool kPushedNeighbor =
        TryPushDfsNeighbor(maze_grid, *kGridSize, kCurrent, visited,
                           parents, frontier, visual_states);

    if (!kPushedNeighbor) {
      frontier.pop();
      visual_states[kCurrent.first][kCurrent.second] =
          SolverCellState::VISITED_PROC;

      if (ShouldEmitBacktrackProgress(parents, kCurrent, kEndpoints)) {
        EmitProgress(execution_state, visual_states, {}, false);
      }
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
