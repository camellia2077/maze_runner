#include "graphics/render_layout.h"

#include <algorithm>

namespace MazeSolver::detail {

auto ComputeImageSize(const Config::MazeConfig& maze) -> ImageSize {
  const int img_width_units = (kGridSpacing * maze.width) + 1;
  const int img_height_units = (kGridSpacing * maze.height) + 1;
  const int final_img_width = img_width_units * maze.unit_pixels;
  const int final_img_height = img_height_units * maze.unit_pixels;
  const auto pixel_count = static_cast<size_t>(final_img_width) *
                           static_cast<size_t>(final_img_height) *
                           static_cast<size_t>(kRgbChannels);
  return {.img_width_units = img_width_units,
          .img_height_units = img_height_units,
          .final_img_width = final_img_width,
          .final_img_height = final_img_height,
          .pixel_count = pixel_count};
}

auto IsValidMazeConfig(const Config::MazeConfig& maze, std::string& error)
    -> bool {
  if (maze.width <= 0 || maze.height <= 0 || maze.unit_pixels <= 0) {
    error =
        "Invalid maze dimensions or unit pixels for saving image. Aborting "
        "save.";
    return false;
  }
  return true;
}

auto IsOuterFrameUnit(UnitCoord unit, const ImageSize& image_size) -> bool {
  return unit.row == 0 || unit.row == image_size.img_height_units - 1 ||
         unit.col == 0 || unit.col == image_size.img_width_units - 1;
}

auto IsCellUnit(UnitCoord unit) -> bool {
  return unit.row % kGridSpacing != 0 && unit.col % kGridSpacing != 0;
}

auto IsVerticalWallUnit(UnitCoord unit) -> bool {
  return unit.row % kGridSpacing != 0 && unit.col % kGridSpacing == 0;
}

auto IsHorizontalWallUnit(UnitCoord unit) -> bool {
  return unit.row % kGridSpacing == 0 && unit.col % kGridSpacing != 0;
}

auto CellToUnit(MazeCoord cell) -> UnitCoord {
  return {.row = (kGridSpacing * cell.row) + 1,
          .col = (kGridSpacing * cell.col) + 1};
}

auto TryGetCorridorUnit(const GridPosition& first, const GridPosition& second)
    -> std::optional<UnitCoord> {
  if (first.first == second.first) {
    const int row_unit = (kGridSpacing * first.first) + 1;
    const int col_unit =
        (kGridSpacing * std::min(first.second, second.second)) + kGridSpacing;
    return UnitCoord{.row = row_unit, .col = col_unit};
  }
  if (first.second == second.second) {
    const int col_unit = (kGridSpacing * first.second) + 1;
    const int row_unit =
        (kGridSpacing * std::min(first.first, second.first)) + kGridSpacing;
    return UnitCoord{.row = row_unit, .col = col_unit};
  }
  return std::nullopt;
}

}  // namespace MazeSolver::detail
