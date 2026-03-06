#include "graphics/render_raster.h"

#include <optional>
#include <vector>

#include "graphics/render_layout.h"
#include "graphics/render_palette.h"

namespace MazeSolver::detail {
namespace {

void PaintUnitPixels(std::vector<unsigned char>& pixels,
                     const ImageContext& context, UnitCoord unit,
                     const unsigned char* color) {
  for (int py = 0; py < context.unit_pixels; ++py) {
    for (int px = 0; px < context.unit_pixels; ++px) {
      const int img_x = (unit.col * context.unit_pixels) + px;
      const int img_y = (unit.row * context.unit_pixels) + py;
      if (img_x < context.final_img_width && img_y < context.final_img_height) {
        const auto pixel_index =
            static_cast<size_t>((img_y * context.final_img_width) + img_x) *
            static_cast<size_t>(kRgbChannels);
        pixels[pixel_index] = color[0];
        pixels[pixel_index + 1] = color[1];
        pixels[pixel_index + 2] = color[2];
      }
    }
  }
}

void PaintCellPixels(std::vector<unsigned char>& pixels,
                     const ImageContext& context, MazeCoord cell,
                     const unsigned char* color) {
  PaintUnitPixels(pixels, context, CellToUnit(cell), color);
}

void PaintBaseImage(std::vector<unsigned char>& pixels,
                    const ImageContext& context, const ImageSize& image_size,
                    const MazeSolverDomain::SearchFrame& frame,
                    const MazeDomain::MazeGrid& maze_ref,
                    const Config::MazeConfig& maze,
                    const Config::ColorConfig& colors) {
  for (int row_unit = 0; row_unit < image_size.img_height_units; ++row_unit) {
    for (int col_unit = 0; col_unit < image_size.img_width_units; ++col_unit) {
      const UnitCoord unit{.row = row_unit, .col = col_unit};
      const unsigned char* current_color_ptr =
          SelectUnitColor(unit, image_size, frame, maze_ref, maze, colors);
      PaintUnitPixels(pixels, context, unit, current_color_ptr);
    }
  }
}

void PaintPathPoints(std::vector<unsigned char>& pixels,
                     const ImageContext& context,
                     const std::vector<GridPosition>& path,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors) {
  for (const auto& point : path) {
    const unsigned char* path_color_ptr = colors.solution_path;
    if (point == maze.start_node) {
      path_color_ptr = colors.start;
    } else if (point == maze.end_node) {
      path_color_ptr = colors.end;
    }
    const MazeCoord cell{.row = point.first, .col = point.second};
    PaintCellPixels(pixels, context, cell, path_color_ptr);
  }
}

void PaintPathCorridors(std::vector<unsigned char>& pixels,
                        const ImageContext& context,
                        const std::vector<GridPosition>& path,
                        const Config::ColorConfig& colors) {
  if (path.size() <= 1) {
    return;
  }

  for (size_t index = 0; index + 1 < path.size(); ++index) {
    const GridPosition first = path[index];
    const GridPosition second = path[index + 1];
    const auto corridor_unit = TryGetCorridorUnit(first, second);
    if (!corridor_unit.has_value()) {
      continue;
    }
    PaintUnitPixels(pixels, context, *corridor_unit, colors.solution_path);
  }
}

}  // namespace

auto RenderFrameToBufferImpl(const MazeSolverDomain::SearchFrame& frame,
                             const MazeDomain::MazeGrid& maze_ref,
                             const Config::AppConfig& config,
                             MazeSolver::RenderBuffer& out_buffer,
                             std::string& error) -> bool {
  const auto& maze = config.maze;
  const auto& colors = config.colors;

  if (!IsValidMazeConfig(maze, error)) {
    return false;
  }

  if (frame.visual_states_.empty()) {
    return true;
  }

  const ImageSize image_size = ComputeImageSize(maze);
  const ImageContext context{
      .final_img_width = image_size.final_img_width,
      .final_img_height = image_size.final_img_height,
      .unit_pixels = maze.unit_pixels};
  std::vector<unsigned char> pixels(image_size.pixel_count);

  PaintBaseImage(pixels, context, image_size, frame, maze_ref, maze, colors);
  PaintPathPoints(pixels, context, frame.current_path_, maze, colors);
  PaintPathCorridors(pixels, context, frame.current_path_, colors);

  out_buffer.width = image_size.final_img_width;
  out_buffer.height = image_size.final_img_height;
  out_buffer.channels = kRgbChannels;
  out_buffer.pixels = std::move(pixels);
  return true;
}

}  // namespace MazeSolver::detail
