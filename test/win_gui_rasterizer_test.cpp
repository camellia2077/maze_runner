#include <iostream>
#include <array>

#include "src/render/maze_rasterizer.h"

namespace {

void ExpectTrue(bool condition, const char* message, int& failures) {
  if (!condition) {
    std::cerr << "[EXPECT_TRUE] " << message << "\n";
    ++failures;
  }
}

template <typename Type>
void ExpectEqual(const Type& left, const Type& right, const char* message,
                 int& failures) {
  if (!(left == right)) {
    std::cerr << "[EXPECT_EQUAL] " << message << "\n";
    ++failures;
  }
}

}  // namespace

auto RunWinGuiRasterizerTests() -> int {
  int failures = 0;

  {
    MazeWinGui::State::RasterStateSnapshot snapshot;
    snapshot.maze_width = 2;
    snapshot.maze_height = 2;
    snapshot.unit_pixels = 2;
    snapshot.start_x = 0;
    snapshot.start_y = 0;
    snapshot.end_x = 1;
    snapshot.end_y = 1;
    snapshot.cell_states = {0, 4, 5, 6};
    snapshot.topology_wall_masks = {15, 15, 15, 15};
    snapshot.path_cells = {{0, 0}, {0, 1}};

    const auto frame = MazeWinGui::Render::MazeRasterizer::Rasterize(snapshot);
    ExpectEqual(frame.width, 10, "frame width should match layout", failures);
    ExpectEqual(frame.height, 10, "frame height should match layout", failures);
    ExpectEqual(frame.channels, 4, "frame channels should be 4", failures);
    ExpectEqual(frame.bgra.size(), static_cast<size_t>(400),
                "frame pixel size should match dimensions", failures);
    if (!frame.bgra.empty()) {
      ExpectEqual(frame.bgra[3], static_cast<unsigned char>(255),
                  "alpha channel should be opaque", failures);
    }
  }

  {
    MazeWinGui::State::RasterStateSnapshot empty_snapshot;
    const auto frame = MazeWinGui::Render::MazeRasterizer::Rasterize(empty_snapshot);
    ExpectEqual(frame.width, 0, "empty snapshot width", failures);
    ExpectTrue(frame.bgra.empty(), "empty snapshot should output empty pixels",
               failures);
  }

  {
    MazeWinGui::State::RasterStateSnapshot initial_snapshot;
    initial_snapshot.maze_width = 2;
    initial_snapshot.maze_height = 2;
    initial_snapshot.unit_pixels = 2;
    initial_snapshot.start_x = 0;
    initial_snapshot.start_y = 0;
    initial_snapshot.end_x = 1;
    initial_snapshot.end_y = 1;
    initial_snapshot.cell_states = {0, 0, 0, 0};
    initial_snapshot.topology_wall_masks = {15, 15, 15, 15};

    const auto initial_frame =
        MazeWinGui::Render::MazeRasterizer::Rasterize(initial_snapshot);

    auto changed_snapshot = initial_snapshot;
    changed_snapshot.cell_states[1] = 4;
    changed_snapshot.path_cells = {{0, 1}};
    auto incremental_bgra = initial_frame.bgra;
    const std::array<MazeWinGui::Event::CellCoord, 1> dirty_cells = {
        MazeWinGui::Event::CellCoord{.row = 0, .col = 1}};
    const bool incremental_ok = MazeWinGui::Render::MazeRasterizer::ApplyCellUpdates(
        changed_snapshot, dirty_cells, incremental_bgra,
        initial_frame.width, initial_frame.height, initial_frame.channels);
    ExpectTrue(incremental_ok, "incremental cell update should succeed", failures);

    const auto rerasterized =
        MazeWinGui::Render::MazeRasterizer::Rasterize(changed_snapshot);
    ExpectEqual(incremental_bgra, rerasterized.bgra,
                "incremental update should match full reraster", failures);
  }

  return failures;
}
