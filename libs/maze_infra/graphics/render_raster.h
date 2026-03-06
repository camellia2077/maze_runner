#ifndef MAZE_INFRA_GRAPHICS_RENDER_RASTER_H
#define MAZE_INFRA_GRAPHICS_RENDER_RASTER_H

#include <string>

#include "config/config.h"
#include "domain/maze_grid.h"
#include "domain/maze_solver.h"
#include "infrastructure/graphics/maze_renderer.h"

namespace MazeSolver::detail {

auto RenderFrameToBufferImpl(const MazeSolverDomain::SearchFrame& frame,
                             const MazeDomain::MazeGrid& maze_ref,
                             const Config::AppConfig& config,
                             MazeSolver::RenderBuffer& out_buffer,
                             std::string& error) -> bool;

}  // namespace MazeSolver::detail

#endif  // MAZE_INFRA_GRAPHICS_RENDER_RASTER_H
