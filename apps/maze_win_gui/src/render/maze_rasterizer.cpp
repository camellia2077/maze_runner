#include "src/render/maze_rasterizer.h"

#include <algorithm>
#include <cstddef>

namespace MazeWinGui::Render {
namespace {

constexpr int kGridSpacing = 2;
constexpr int kWallTopBit = 1 << 0;
constexpr int kWallRightBit = 1 << 1;
constexpr int kWallBottomBit = 1 << 2;
constexpr int kWallLeftBit = 1 << 3;

struct BgraColor {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
};

auto CellIndex(const State::RasterStateSnapshot& state, int row, int col)
    -> size_t {
  return static_cast<size_t>(row * state.maze_width + col);
}

auto CellWallMask(const State::RasterStateSnapshot& state, int row, int col)
    -> int {
  if (row < 0 || row >= state.maze_height || col < 0 || col >= state.maze_width) {
    return kWallTopBit | kWallRightBit | kWallBottomBit | kWallLeftBit;
  }
  const size_t idx = CellIndex(state, row, col);
  if (idx >= state.topology_wall_masks.size()) {
    return kWallTopBit | kWallRightBit | kWallBottomBit | kWallLeftBit;
  }
  return static_cast<int>(state.topology_wall_masks[idx]);
}

auto ExpectedFrameWidth(const State::RasterStateSnapshot& state) -> int {
  return ((kGridSpacing * state.maze_width) + 1) * state.unit_pixels;
}

auto ExpectedFrameHeight(const State::RasterStateSnapshot& state) -> int {
  return ((kGridSpacing * state.maze_height) + 1) * state.unit_pixels;
}

auto StateColor(int state, bool is_start, bool is_end) -> BgraColor {
  if (is_start) {
    return {.b = 219, .g = 152, .r = 52, .a = 255};
  }
  if (is_end) {
    return {.b = 52, .g = 73, .r = 219, .a = 255};
  }
  switch (state) {
    case 3:
      return {.b = 246, .g = 196, .r = 90, .a = 255};
    case 4:
      return {.b = 66, .g = 165, .r = 245, .a = 255};
    case 5:
      return {.b = 176, .g = 176, .r = 176, .a = 255};
    case 6:
      return {.b = 76, .g = 175, .r = 80, .a = 255};
    default:
      return {.b = 245, .g = 245, .r = 245, .a = 255};
  }
}

void PaintUnit(std::vector<unsigned char>& bgra, int frame_width, int unit_pixels,
               int unit_row, int unit_col, const BgraColor& color) {
  const int pixel_left = unit_col * unit_pixels;
  const int pixel_top = unit_row * unit_pixels;
  for (int py = 0; py < unit_pixels; ++py) {
    const int y = pixel_top + py;
    for (int px = 0; px < unit_pixels; ++px) {
      const int x = pixel_left + px;
      const size_t offset = static_cast<size_t>((y * frame_width) + x) * 4;
      bgra[offset + 0] = color.b;
      bgra[offset + 1] = color.g;
      bgra[offset + 2] = color.r;
      bgra[offset + 3] = color.a;
    }
  }
}

auto IsPathCell(const State::RasterStateSnapshot& state, int row, int col)
    -> bool {
  for (const auto& cell : state.path_cells) {
    if (cell.row == row && cell.col == col) {
      return true;
    }
  }
  return false;
}

auto ResolveCellColor(const State::RasterStateSnapshot& state, int row, int col)
    -> BgraColor {
  const size_t idx = CellIndex(state, row, col);
  const bool is_start = (col == state.start_x && row == state.start_y);
  const bool is_end = (col == state.end_x && row == state.end_y);
  BgraColor color = StateColor(state.cell_states[idx], is_start, is_end);
  if (!is_start && !is_end && IsPathCell(state, row, col)) {
    color = {.b = 67, .g = 160, .r = 255, .a = 255};
  }
  return color;
}

}  // namespace

auto MazeRasterizer::Rasterize(const State::RasterStateSnapshot& state)
    -> RasterizedFrame {
  RasterizedFrame frame;
  if (state.maze_width <= 0 || state.maze_height <= 0 || state.unit_pixels <= 0 ||
      state.cell_states.empty()) {
    return frame;
  }

  const int img_width_units = (kGridSpacing * state.maze_width) + 1;
  const int img_height_units = (kGridSpacing * state.maze_height) + 1;
  frame.width = ExpectedFrameWidth(state);
  frame.height = ExpectedFrameHeight(state);
  frame.channels = 4;

  const size_t pixel_count =
      static_cast<size_t>(frame.width) * static_cast<size_t>(frame.height);
  frame.bgra.assign(pixel_count * static_cast<size_t>(frame.channels), 0);

  const BgraColor background = {.b = 245, .g = 245, .r = 245, .a = 255};
  const BgraColor inner_wall = {.b = 40, .g = 40, .r = 40, .a = 255};
  const BgraColor outer_wall = {.b = 20, .g = 20, .r = 20, .a = 255};

  for (int row_unit = 0; row_unit < img_height_units; ++row_unit) {
    for (int col_unit = 0; col_unit < img_width_units; ++col_unit) {
      const bool is_outer =
          (row_unit == 0 || row_unit == img_height_units - 1 || col_unit == 0 ||
           col_unit == img_width_units - 1);
      if (is_outer) {
        PaintUnit(frame.bgra, frame.width, state.unit_pixels, row_unit, col_unit,
                  outer_wall);
        continue;
      }

      const bool is_cell_unit =
          (row_unit % kGridSpacing != 0 && col_unit % kGridSpacing != 0);
      if (is_cell_unit) {
        const int row = (row_unit - 1) / kGridSpacing;
        const int col = (col_unit - 1) / kGridSpacing;
        PaintUnit(frame.bgra, frame.width, state.unit_pixels, row_unit, col_unit,
                  ResolveCellColor(state, row, col));
        continue;
      }

      const bool is_vertical_wall_unit =
          (row_unit % kGridSpacing != 0 && col_unit % kGridSpacing == 0);
      if (is_vertical_wall_unit) {
        const int row = (row_unit - 1) / kGridSpacing;
        const int col_left = (col_unit / kGridSpacing) - 1;
        bool blocked = true;
        if (row >= 0 && row < state.maze_height && col_left >= 0 &&
            col_left < state.maze_width) {
          blocked = (CellWallMask(state, row, col_left) & kWallRightBit) != 0;
        }
        PaintUnit(frame.bgra, frame.width, state.unit_pixels, row_unit, col_unit,
                  blocked ? inner_wall : background);
        continue;
      }

      const bool is_horizontal_wall_unit =
          (row_unit % kGridSpacing == 0 && col_unit % kGridSpacing != 0);
      if (is_horizontal_wall_unit) {
        const int row_up = (row_unit / kGridSpacing) - 1;
        const int col = (col_unit - 1) / kGridSpacing;
        bool blocked = true;
        if (row_up >= 0 && row_up < state.maze_height && col >= 0 &&
            col < state.maze_width) {
          blocked = (CellWallMask(state, row_up, col) & kWallBottomBit) != 0;
        }
        PaintUnit(frame.bgra, frame.width, state.unit_pixels, row_unit, col_unit,
                  blocked ? inner_wall : background);
        continue;
      }

      PaintUnit(frame.bgra, frame.width, state.unit_pixels, row_unit, col_unit,
                inner_wall);
    }
  }

  return frame;
}

auto MazeRasterizer::ApplyCellUpdates(
    const State::RasterStateSnapshot& state,
    std::span<const Event::CellCoord> dirty_cells, std::vector<unsigned char>& bgra,
    int width, int height, int channels) -> bool {
  if (dirty_cells.empty()) {
    return true;
  }
  if (width != ExpectedFrameWidth(state) || height != ExpectedFrameHeight(state) ||
      channels != 4) {
    return false;
  }
  const size_t expected_size =
      static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
  if (bgra.size() != expected_size) {
    return false;
  }

  for (const auto& cell : dirty_cells) {
    if (cell.row < 0 || cell.row >= state.maze_height || cell.col < 0 ||
        cell.col >= state.maze_width) {
      continue;
    }
    const int unit_row = (cell.row * kGridSpacing) + 1;
    const int unit_col = (cell.col * kGridSpacing) + 1;
    PaintUnit(bgra, width, state.unit_pixels, unit_row, unit_col,
              ResolveCellColor(state, cell.row, cell.col));
  }
  return true;
}

}  // namespace MazeWinGui::Render
