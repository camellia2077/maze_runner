#ifndef MAZE_DOMAIN_MAZE_SOLVER_NEIGHBORS_H
#define MAZE_DOMAIN_MAZE_SOLVER_NEIGHBORS_H

#include "domain/maze_solver_common.h"

namespace MazeSolverDomain::detail {

auto ReachableNeighbors(const MazeGrid& maze_grid, GridSize grid_size,
                        GridPosition current, NeighborOrder order)
    -> std::vector<GridPosition>;

}  // namespace MazeSolverDomain::detail

#endif  // MAZE_DOMAIN_MAZE_SOLVER_NEIGHBORS_H
