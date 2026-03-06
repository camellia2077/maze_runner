#include "domain/maze_solver_result_builder.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace MazeSolverDomain::detail {
namespace {

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
  const int delta_r1 = line.second.first - line.first.first;
  const int delta_c1 = line.second.second - line.first.second;
  const int delta_r2 = line.third.first - line.second.first;
  const int delta_c2 = line.third.second - line.second.second;
  return delta_r1 == delta_r2 && delta_c1 == delta_c2;
}

auto ShouldSkipStraightLineProgress(GridSize grid_size, const ParentGrid& parents,
                                    GridPosition current) -> bool {
  const MazeDomain::CellId parent_id = parents[current.first][current.second];
  if (parent_id == kInvalidCellId) {
    return false;
  }
  const GridPosition parent_cell = ToGridPosition(grid_size, parent_id);
  if (parent_cell == kInvalidCell) {
    return false;
  }
  const MazeDomain::CellId grandparent_id =
      parents[parent_cell.first][parent_cell.second];
  if (grandparent_id == kInvalidCellId) {
    return false;
  }
  const GridPosition grandparent_cell = ToGridPosition(grid_size, grandparent_id);
  if (grandparent_cell == kInvalidCell) {
    return false;
  }
  const StraightLineTriplet line{
      .first = grandparent_cell, .second = parent_cell, .third = current};
  return IsStraightLine(line);
}

void AppendSolutionPath(GridSize grid_size, const PathEndpoints& endpoints,
                        const ParentGrid& parents, StateGrid& visual_states,
                        SolveExecutionState& execution_state,
                        SearchResult& result) {
  std::vector<GridPosition> path;
  GridPosition path_node = endpoints.end;
  while (path_node != kInvalidCell) {
    path.push_back(path_node);
    visual_states[path_node.first][path_node.second] = SolverCellState::SOLUTION;
    if (path_node == endpoints.start) {
      break;
    }
    const MazeDomain::CellId parent_id = parents[path_node.first][path_node.second];
    path_node = ToGridPosition(grid_size, parent_id);
  }
  std::ranges::reverse(path);
  result.path_ = path;
  EmitProgress(execution_state, visual_states, path, true);

  SearchEvent path_event;
  path_event.type = SearchEventType::kPathUpdated;
  path_event.path = path;
  path_event.found = true;
  if (execution_state.sink != nullptr) {
    path_event.seq = execution_state.next_seq++;
    execution_state.sink->OnEvent(path_event);
  }
}

}  // namespace

auto ShouldEmitProgressForCurrent(const ParentGrid& parents, GridPosition current,
                                  const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.end) {
    return true;
  }
  const GridSize grid_size{
      .height = static_cast<int>(parents.size()),
      .width = parents.empty() ? 0 : static_cast<int>(parents.front().size())};
  return !ShouldSkipStraightLineProgress(grid_size, parents, current);
}

auto ShouldEmitBacktrackProgress(const ParentGrid& parents, GridPosition current,
                                 const PathEndpoints& endpoints) -> bool {
  if (current == endpoints.start || current == endpoints.end) {
    return true;
  }
  const GridSize grid_size{
      .height = static_cast<int>(parents.size()),
      .width = parents.empty() ? 0 : static_cast<int>(parents.front().size())};
  return !ShouldSkipStraightLineProgress(grid_size, parents, current);
}

auto CreateTrivialResult(GridSize grid_size, GridPosition node,
                         SolveExecutionState& execution_state) -> SearchResult {
  SearchResult result;
  auto visual_states = CreateStateGrid(grid_size, SolverCellState::NONE);
  auto visited = CreateBoolGrid(grid_size, false);
  visited[node.first][node.second] = true;
  visual_states[node.first][node.second] = SolverCellState::SOLUTION;
  result.path_.push_back(node);
  EmitProgress(execution_state, visual_states, result.path_, true);
  result.explored_ = std::move(visited);
  result.found_ = true;
  EmitRunFinished(execution_state, result);
  return result;
}

void FinalizeSearchResult(bool found, const PathEndpoints& endpoints,
                          const ParentGrid& parents, StateGrid& visual_states,
                          BoolGrid&& visited,
                          SolveExecutionState& execution_state,
                          SearchResult& result) {
  const GridSize grid_size{
      .height = static_cast<int>(parents.size()),
      .width = parents.empty() ? 0 : static_cast<int>(parents.front().size())};
  if (found) {
    AppendSolutionPath(grid_size, endpoints, parents, visual_states,
                       execution_state, result);
  } else {
    EmitProgress(execution_state, visual_states, {}, true);
  }

  result.found_ = found;
  result.explored_ = std::move(visited);
  EmitRunFinished(execution_state, result);
}

}  // namespace MazeSolverDomain::detail
