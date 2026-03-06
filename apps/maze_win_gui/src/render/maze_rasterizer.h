#pragma once

#include <span>
#include <vector>

#include "src/state/gui_state_store.h"

namespace MazeWinGui::Render {

struct RasterizedFrame {
  int width = 0;
  int height = 0;
  int channels = 0;
  std::vector<unsigned char> bgra;
};

class MazeRasterizer {
 public:
  static auto Rasterize(const State::RasterStateSnapshot& state)
      -> RasterizedFrame;
  static auto ApplyCellUpdates(
      const State::RasterStateSnapshot& state,
      std::span<const Event::CellCoord> dirty_cells,
      std::vector<unsigned char>& bgra, int width, int height,
      int channels) -> bool;
};

}  // namespace MazeWinGui::Render
