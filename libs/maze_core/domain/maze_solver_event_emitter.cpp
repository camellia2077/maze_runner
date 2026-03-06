#include "domain/maze_solver_event_emitter.h"

#include <algorithm>
#include <string_view>
#include <utility>

namespace MazeSolverDomain::detail {
namespace {

auto CountState(const StateGrid& visual_states, SolverCellState state) -> int {
  int count = 0;
  for (const auto& row : visual_states) {
    for (const SolverCellState cell_state : row) {
      if (cell_state == state) {
        ++count;
      }
    }
  }
  return count;
}

auto BuildDeltas(const StateGrid& previous, const StateGrid& current,
                 bool has_previous) -> std::vector<CellDelta> {
  std::vector<CellDelta> deltas;
  if (current.empty()) {
    return deltas;
  }

  const int height = static_cast<int>(current.size());
  const int width = static_cast<int>(current.front().size());
  deltas.reserve(static_cast<size_t>(height * width) / 4 + 4);
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      const SolverCellState current_state = current[row][col];
      if (!has_previous) {
        if (current_state != SolverCellState::NONE) {
          deltas.push_back({.row = row, .col = col, .state = current_state});
        }
        continue;
      }
      const SolverCellState previous_state = previous[row][col];
      if (current_state != previous_state) {
        deltas.push_back({.row = row, .col = col, .state = current_state});
      }
    }
  }
  return deltas;
}

void EmitEvent(SolveExecutionState& execution_state, SearchEvent event) {
  if (execution_state.sink == nullptr) {
    return;
  }
  event.seq = execution_state.next_seq++;
  execution_state.sink->OnEvent(event);
}

}  // namespace

auto CreateExecutionState(ISearchEventSink* sink, const SolveOptions& options)
    -> SolveExecutionState {
  SolveExecutionState execution_state;
  execution_state.sink = sink;
  execution_state.options = options;
  if (execution_state.options.emit_stride <= 0) {
    execution_state.options.emit_stride = 1;
  }
  return execution_state;
}

auto ShouldCancel(const SolveExecutionState& execution_state) -> bool {
  return execution_state.sink != nullptr && execution_state.sink->ShouldCancel();
}

void EmitRunStarted(SolveExecutionState& execution_state) {
  SearchEvent event;
  event.type = SearchEventType::kRunStarted;
  EmitEvent(execution_state, std::move(event));
}

void EmitRunFailed(SolveExecutionState& execution_state,
                   std::string_view message) {
  SearchEvent event;
  event.type = SearchEventType::kRunFailed;
  event.message.assign(message);
  EmitEvent(execution_state, std::move(event));
}

void EmitRunCancelled(SolveExecutionState& execution_state,
                      std::string_view message) {
  SearchEvent event;
  event.type = SearchEventType::kRunCancelled;
  event.message.assign(message);
  EmitEvent(execution_state, std::move(event));
}

void EmitRunFinished(SolveExecutionState& execution_state,
                     const SearchResult& result) {
  SearchEvent event;
  event.type = SearchEventType::kRunFinished;
  event.found = result.found_;
  event.path = result.path_;
  EmitEvent(execution_state, std::move(event));
}

void EmitProgress(SolveExecutionState& execution_state,
                  const StateGrid& visual_states,
                  const std::vector<GridPosition>& current_path,
                  bool force_emit) {
  execution_state.step_counter += 1;
  if (!force_emit) {
    if (!execution_state.options.emit_progress) {
      return;
    }
    const int stride = std::max(1, execution_state.options.emit_stride);
    if ((execution_state.step_counter - 1) % stride != 0) {
      return;
    }
  }

  SearchEvent event;
  event.type = SearchEventType::kProgress;
  event.path = current_path;
  event.deltas = BuildDeltas(execution_state.last_emitted_states, visual_states,
                             execution_state.has_emitted_state);
  event.visited_count = CountState(visual_states, SolverCellState::VISITED_PROC);
  event.frontier_count = CountState(visual_states, SolverCellState::FRONTIER);
  EmitEvent(execution_state, event);

  execution_state.last_emitted_states = visual_states;
  execution_state.has_emitted_state = true;
}

}  // namespace MazeSolverDomain::detail
