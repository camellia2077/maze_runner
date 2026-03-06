#include <iostream>
#include <string>

#include "api/maze_api.h"
#include "src/state/gui_state_store.h"

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

auto RunWinGuiStateStoreTests() -> int {
  int failures = 0;
  MazeWinGui::State::GuiStateStore store;

  const MazeWinGui::State::RunParams params{
      .width = 2,
      .height = 2,
      .unit_pixels = 4,
      .start_x = 0,
      .start_y = 0,
      .end_x = 1,
      .end_y = 1};

  {
    const bool started = store.TryBeginRun(params);
    ExpectTrue(started, "TryBeginRun should succeed", failures);
    const auto paint = store.GetPaintSnapshot();
    ExpectTrue(paint.running, "running should be true after begin", failures);
    const auto raster = store.GetRasterStateSnapshot();
    ExpectEqual(raster.maze_width, 2, "raster width", failures);
    ExpectEqual(raster.maze_height, 2, "raster height", failures);
    ExpectEqual(raster.cell_states.size(), static_cast<size_t>(4),
                "cell state count", failures);
  }

  {
    const std::string topology_payload =
        R"({"width":2,"height":2,"start":{"row":0,"col":0},"end":{"row":1,"col":1},"walls":[15,14,13,12]})";
    const auto raster_plan =
        store.ApplyEvent(MAZE_EVENT_TOPOLOGY_SNAPSHOT, topology_payload);
    ExpectTrue(raster_plan.requires_full_raster,
               "topology event should require raster rebuild",
               failures);
    const auto raster = store.GetRasterStateSnapshot();
    ExpectEqual(raster.topology_wall_masks.size(), static_cast<size_t>(4),
                "topology wall count", failures);
  }

  {
    const std::string progress_payload =
        R"({"deltas":[{"row":0,"col":1,"state":4}],"path":[{"row":0,"col":0},{"row":0,"col":1}]})";
    const auto raster_plan =
        store.ApplyEvent(MAZE_EVENT_PROGRESS, progress_payload);
    ExpectTrue(!raster_plan.requires_full_raster,
               "progress should stay on incremental raster path", failures);
    ExpectEqual(raster_plan.dirty_cells.size(), static_cast<size_t>(2),
                "progress should mark path cells dirty", failures);
    const auto raster = store.GetRasterStateSnapshot();
    ExpectEqual(raster.path_cells.size(), static_cast<size_t>(2), "path count",
                failures);
    if (raster.cell_states.size() >= 2) {
      ExpectEqual(raster.cell_states[1], 4, "delta should update state", failures);
    }
    const auto paint = store.GetPaintSnapshot();
    ExpectEqual(paint.frame_count, 1, "frame_count should increase", failures);
  }

  {
    store.MarkCancelRequestedResult(MAZE_STATUS_OK);
    const auto paint = store.GetPaintSnapshot();
    ExpectTrue(paint.cancel_requested, "cancel_requested should be true", failures);
    ExpectEqual(paint.status, std::string("Cancelling..."), "cancel status",
                failures);
  }

  {
    store.MarkRunCompleted(MAZE_STATUS_OK, nullptr, "{\"ok\":1}");
    const auto paint = store.GetPaintSnapshot();
    ExpectTrue(!paint.running, "running should be false after completed", failures);
    ExpectEqual(paint.status, std::string("Run success."), "run success status",
                failures);
    ExpectEqual(paint.frame_count, 1, "frame_count should preserve final progress",
                failures);
    ExpectEqual(paint.summary_json, std::string("{\"ok\":1}"), "summary json",
                failures);
  }

  return failures;
}
