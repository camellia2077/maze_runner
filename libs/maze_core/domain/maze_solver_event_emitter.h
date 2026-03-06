#ifndef MAZE_DOMAIN_MAZE_SOLVER_EVENT_EMITTER_H
#define MAZE_DOMAIN_MAZE_SOLVER_EVENT_EMITTER_H

#include <string_view>
#include <vector>

#include "domain/maze_solver_common.h"

namespace MazeSolverDomain::detail {

auto CreateExecutionState(ISearchEventSink* sink, const SolveOptions& options)
    -> SolveExecutionState;
auto ShouldCancel(const SolveExecutionState& execution_state) -> bool;
void EmitRunStarted(SolveExecutionState& execution_state);
void EmitRunFailed(SolveExecutionState& execution_state,
                   std::string_view message);
void EmitRunCancelled(SolveExecutionState& execution_state,
                      std::string_view message);
void EmitRunFinished(SolveExecutionState& execution_state,
                     const SearchResult& result);
void EmitProgress(SolveExecutionState& execution_state,
                  const StateGrid& visual_states,
                  const std::vector<GridPosition>& current_path, bool force_emit);

}  // namespace MazeSolverDomain::detail

#endif  // MAZE_DOMAIN_MAZE_SOLVER_EVENT_EMITTER_H
