#ifndef MAZE_INFRA_GRAPHICS_RENDER_OUTPUT_PNG_H
#define MAZE_INFRA_GRAPHICS_RENDER_OUTPUT_PNG_H

#include <string>

#include "infrastructure/graphics/maze_renderer.h"

namespace MazeSolver::detail {

auto WritePngFileImpl(const std::string& path, const MazeSolver::RenderBuffer& buffer,
                      std::string& error) -> bool;

}  // namespace MazeSolver::detail

#endif  // MAZE_INFRA_GRAPHICS_RENDER_OUTPUT_PNG_H
