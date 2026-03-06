#ifndef MAZE_INFRA_GRAPHICS_RENDER_LAYOUT_H
#define MAZE_INFRA_GRAPHICS_RENDER_LAYOUT_H

#include <optional>
#include <string>

#include "config/config.h"
#include "domain/maze_solver.h"

namespace MazeSolver::detail {

using GridPosition = MazeSolverDomain::GridPosition;

inline constexpr int kRgbChannels = 3;
inline constexpr int kGridSpacing = 2;

struct ImageSize {
  int img_width_units = 0;
  int img_height_units = 0;
  int final_img_width = 0;
  int final_img_height = 0;
  size_t pixel_count = 0;
};

struct ImageContext {
  int final_img_width = 0;
  int final_img_height = 0;
  int unit_pixels = 0;
};

struct UnitCoord {
  int row = 0;
  int col = 0;
};

struct MazeCoord {
  int row = 0;
  int col = 0;
};

auto ComputeImageSize(const Config::MazeConfig& maze) -> ImageSize;
auto IsValidMazeConfig(const Config::MazeConfig& maze, std::string& error)
    -> bool;
auto IsOuterFrameUnit(UnitCoord unit, const ImageSize& image_size) -> bool;
auto IsCellUnit(UnitCoord unit) -> bool;
auto IsVerticalWallUnit(UnitCoord unit) -> bool;
auto IsHorizontalWallUnit(UnitCoord unit) -> bool;
auto CellToUnit(MazeCoord cell) -> UnitCoord;
auto TryGetCorridorUnit(const GridPosition& first, const GridPosition& second)
    -> std::optional<UnitCoord>;

}  // namespace MazeSolver::detail

#endif  // MAZE_INFRA_GRAPHICS_RENDER_LAYOUT_H
