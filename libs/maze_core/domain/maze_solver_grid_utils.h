#ifndef MAZE_DOMAIN_MAZE_SOLVER_GRID_UTILS_H
#define MAZE_DOMAIN_MAZE_SOLVER_GRID_UTILS_H

#include "domain/maze_solver_common.h"

namespace MazeSolverDomain::detail {

auto GetGridSize(const MazeGrid& maze_grid) -> std::optional<GridSize>;
auto IsValidPosition(GridPosition pos, GridSize grid_size) -> bool;
auto CreateBoolGrid(GridSize grid_size, bool initial) -> BoolGrid;
auto CreateIntGrid(GridSize grid_size, int initial) -> IntGrid;
auto CreateStateGrid(GridSize grid_size, SolverCellState initial) -> StateGrid;
auto CreateParentGrid(GridSize grid_size, MazeDomain::CellId initial)
    -> ParentGrid;
auto ToCellId(GridSize grid_size, GridPosition pos) -> MazeDomain::CellId;
auto ToGridPosition(GridSize grid_size, MazeDomain::CellId cell_id)
    -> GridPosition;
auto ManhattanDistance(PositionPair positions) -> int;

}  // namespace MazeSolverDomain::detail

#endif  // MAZE_DOMAIN_MAZE_SOLVER_GRID_UTILS_H
