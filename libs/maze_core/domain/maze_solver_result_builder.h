#ifndef MAZE_DOMAIN_MAZE_SOLVER_RESULT_BUILDER_H
#define MAZE_DOMAIN_MAZE_SOLVER_RESULT_BUILDER_H

#include "domain/maze_solver_common.h"

namespace MazeSolverDomain::detail {

auto ShouldEmitProgressForCurrent(const ParentGrid& parents, GridPosition current,
                                  const PathEndpoints& endpoints) -> bool;
auto ShouldEmitBacktrackProgress(const ParentGrid& parents, GridPosition current,
                                 const PathEndpoints& endpoints) -> bool;
auto CreateTrivialResult(GridSize grid_size, GridPosition node,
                         SolveExecutionState& execution_state) -> SearchResult;
void FinalizeSearchResult(bool found, const PathEndpoints& endpoints,
                          const ParentGrid& parents, StateGrid& visual_states,
                          BoolGrid&& visited,
                          SolveExecutionState& execution_state,
                          SearchResult& result);

}  // namespace MazeSolverDomain::detail

#endif  // MAZE_DOMAIN_MAZE_SOLVER_RESULT_BUILDER_H
