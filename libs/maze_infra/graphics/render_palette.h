#ifndef MAZE_INFRA_GRAPHICS_RENDER_PALETTE_H
#define MAZE_INFRA_GRAPHICS_RENDER_PALETTE_H

#include "config/config.h"
#include "domain/maze_grid.h"
#include "domain/maze_solver.h"
#include "graphics/render_layout.h"

namespace MazeSolver::detail {

using SearchFrame = MazeSolverDomain::SearchFrame;
using SolverCellState = MazeSolverDomain::SolverCellState;

auto SelectCellColor(MazeCoord cell, const SearchFrame& frame,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors)
    -> const unsigned char*;
auto SelectVerticalWallColor(UnitCoord unit,
                             const MazeDomain::MazeGrid& maze_ref,
                             const Config::MazeConfig& maze,
                             const Config::ColorConfig& colors)
    -> const unsigned char*;
auto SelectHorizontalWallColor(UnitCoord unit,
                               const MazeDomain::MazeGrid& maze_ref,
                               const Config::MazeConfig& maze,
                               const Config::ColorConfig& colors)
    -> const unsigned char*;
auto SelectUnitColor(UnitCoord unit, const ImageSize& image_size,
                     const SearchFrame& frame,
                     const MazeDomain::MazeGrid& maze_ref,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors)
    -> const unsigned char*;

}  // namespace MazeSolver::detail

#endif  // MAZE_INFRA_GRAPHICS_RENDER_PALETTE_H
