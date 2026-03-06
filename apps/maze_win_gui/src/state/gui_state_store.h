#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "src/event/maze_event_parser.h"

namespace MazeWinGui::State {

struct RunParams {
  int width = 0;
  int height = 0;
  int unit_pixels = 0;
  int start_x = 0;
  int start_y = 0;
  int end_x = 0;
  int end_y = 0;
};

struct RasterStateSnapshot {
  int maze_width = 0;
  int maze_height = 0;
  int unit_pixels = 0;
  int start_x = 0;
  int start_y = 0;
  int end_x = 0;
  int end_y = 0;
  std::vector<int> cell_states;
  std::vector<unsigned char> topology_wall_masks;
  std::vector<Event::CellCoord> path_cells;
};

struct PaintSnapshot {
  std::string status;
  std::string summary_json;
  bool running = false;
  bool cancel_requested = false;
  int frame_count = 0;
  int callback_count = 0;
  bool has_topology_snapshot = false;
  int last_width = 0;
  int last_height = 0;
  int last_channels = 0;
  std::vector<unsigned char> latest_bgra;
};

struct RasterUpdatePlan {
  bool requires_full_raster = false;
  std::vector<Event::CellCoord> dirty_cells;
};

class GuiStateStore {
 public:
  GuiStateStore() = default;
  ~GuiStateStore() = default;

  void SetStatus(std::string status);
  auto TryBeginRun(const RunParams& params) -> bool;
  void AbortRun(const std::string& status);
  void MarkRunCompleted(int final_run_code, const char* error,
                        const char* summary_json);
  void MarkCancelRequestedResult(int cancel_code);
  auto ApplyEvent(int event_type, std::string_view payload) -> RasterUpdatePlan;

  auto GetRasterStateSnapshot() const -> RasterStateSnapshot;
  void UpdateRasterFrame(std::vector<unsigned char> bgra, int width, int height,
                         int channels);
  auto MutateRasterFrame(
      const std::function<bool(std::vector<unsigned char>&, int, int, int)>&
          mutator) -> bool;
  auto GetPaintSnapshot() const -> PaintSnapshot;

 private:
  auto CellIndexLocked(int row, int col) const -> size_t;
  void ResetSearchSurfaceLocked(const RunParams& params);

  mutable std::mutex mutex_;
  bool running_ = false;
  bool cancel_requested_ = false;
  int frame_count_ = 0;
  int callback_count_ = 0;
  int maze_width_ = 0;
  int maze_height_ = 0;
  int unit_pixels_ = 0;
  int start_x_ = 0;
  int start_y_ = 0;
  int end_x_ = 0;
  int end_y_ = 0;
  int last_width_ = 0;
  int last_height_ = 0;
  int last_channels_ = 0;
  std::string status_ = "Ready. Configure params and click 'Run JSON'.";
  std::string summary_json_;
  std::vector<int> cell_states_;
  std::vector<unsigned char> topology_wall_masks_;
  bool has_topology_snapshot_ = false;
  std::vector<Event::CellCoord> path_cells_;
  std::vector<unsigned char> latest_bgra_;
};

}  // namespace MazeWinGui::State
