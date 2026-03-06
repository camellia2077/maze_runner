#include "src/state/gui_state_store.h"

#include <algorithm>
#include <array>
#include <utility>

#include "api/maze_api.h"

namespace MazeWinGui::State {
namespace {

constexpr int kWallTopBit = 1 << 0;
constexpr int kWallRightBit = 1 << 1;
constexpr int kWallBottomBit = 1 << 2;
constexpr int kWallLeftBit = 1 << 3;

}  // namespace

void GuiStateStore::SetStatus(std::string status) {
  std::lock_guard<std::mutex> lock(mutex_);
  status_ = std::move(status);
}

auto GuiStateStore::TryBeginRun(const RunParams& params) -> bool {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) {
    status_ = "A task is already running.";
    return false;
  }
  running_ = true;
  cancel_requested_ = false;
  frame_count_ = 0;
  callback_count_ = 0;
  last_width_ = 0;
  last_height_ = 0;
  last_channels_ = 0;
  summary_json_.clear();
  ResetSearchSurfaceLocked(params);
  status_ = "Running...";
  return true;
}

void GuiStateStore::AbortRun(const std::string& status) {
  std::lock_guard<std::mutex> lock(mutex_);
  running_ = false;
  status_ = status;
}

void GuiStateStore::MarkRunCompleted(int final_run_code, const char* error,
                                     const char* summary_json) {
  std::lock_guard<std::mutex> lock(mutex_);
  summary_json_ = (summary_json != nullptr) ? summary_json : "";
  running_ = false;
  if (final_run_code == MAZE_STATUS_OK) {
    status_ = "Run success.";
  } else if (final_run_code == MAZE_STATUS_CANCELLED) {
    status_ = "Run cancelled.";
  } else {
    status_ = std::string("Run failed: ") + ((error != nullptr) ? error : "");
  }
}

void GuiStateStore::MarkCancelRequestedResult(int cancel_code) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (cancel_code == MAZE_STATUS_OK) {
    cancel_requested_ = true;
    status_ = "Cancelling...";
  } else {
    status_ = "Cancel request failed.";
  }
}

auto GuiStateStore::ApplyEvent(int event_type, std::string_view payload)
    -> RasterUpdatePlan {
  std::lock_guard<std::mutex> lock(mutex_);
  RasterUpdatePlan plan;
  callback_count_ += 1;

  const auto mark_dirty = [&](int row, int col) {
    if (row < 0 || row >= maze_height_ || col < 0 || col >= maze_width_) {
      return;
    }
    const auto duplicate = std::find_if(
        plan.dirty_cells.begin(), plan.dirty_cells.end(),
        [&](const Event::CellCoord& cell) { return cell.row == row && cell.col == col; });
    if (duplicate == plan.dirty_cells.end()) {
      plan.dirty_cells.push_back({.row = row, .col = col});
    }
  };

  if (event_type == MAZE_EVENT_TOPOLOGY_SNAPSHOT) {
    Event::TopologySnapshot topology;
    if (!Event::ParseTopologySnapshot(payload, topology)) {
      status_ = "Topology snapshot parse failed.";
      return plan;
    }
    const size_t expected_cells =
        static_cast<size_t>(topology.width) * static_cast<size_t>(topology.height);
    maze_width_ = topology.width;
    maze_height_ = topology.height;
    start_x_ = topology.start.col;
    start_y_ = topology.start.row;
    end_x_ = topology.end.col;
    end_y_ = topology.end.row;
    if (cell_states_.size() != expected_cells) {
      cell_states_.assign(expected_cells, 0);
    }
    topology_wall_masks_ = std::move(topology.wall_masks);
    has_topology_snapshot_ = true;
    plan.requires_full_raster = true;
    return plan;
  }

  if (event_type == MAZE_EVENT_RUN_STARTED) {
    status_ = "Run started.";
    return plan;
  }

  if (event_type == MAZE_EVENT_CELL_STATE_CHANGED) {
    const std::vector<Event::CellDelta> deltas = Event::ParseDeltas(payload);
    for (const auto& delta : deltas) {
      if (delta.row < 0 || delta.row >= maze_height_ || delta.col < 0 ||
          delta.col >= maze_width_) {
        continue;
      }
      const size_t idx = CellIndexLocked(delta.row, delta.col);
      if (idx < cell_states_.size()) {
        cell_states_[idx] = delta.state;
        mark_dirty(delta.row, delta.col);
      }
    }
    return plan;
  }

  if (event_type == MAZE_EVENT_PROGRESS) {
    const std::vector<Event::CellDelta> deltas = Event::ParseDeltas(payload);
    const std::vector<Event::CellCoord> path = Event::ParsePath(payload);
    const auto previous_path = path_cells_;
    for (const auto& delta : deltas) {
      if (delta.row < 0 || delta.row >= maze_height_ || delta.col < 0 ||
          delta.col >= maze_width_) {
        continue;
      }
      const size_t idx = CellIndexLocked(delta.row, delta.col);
      if (idx < cell_states_.size()) {
        cell_states_[idx] = delta.state;
        mark_dirty(delta.row, delta.col);
      }
    }
    path_cells_ = path;
    for (const auto& cell : previous_path) {
      mark_dirty(cell.row, cell.col);
    }
    for (const auto& cell : path_cells_) {
      mark_dirty(cell.row, cell.col);
    }
    frame_count_ += 1;
    if (!cancel_requested_) {
      status_ = "Running...";
    }
    return plan;
  }

  if (event_type == MAZE_EVENT_PATH_UPDATED) {
    const auto previous_path = path_cells_;
    path_cells_ = Event::ParsePath(payload);
    for (const auto& cell : previous_path) {
      mark_dirty(cell.row, cell.col);
    }
    for (const auto& cell : path_cells_) {
      mark_dirty(cell.row, cell.col);
    }
    return plan;
  }

  if (event_type == MAZE_EVENT_RUN_FINISHED) {
    status_ = "Run finished.";
    return plan;
  }
  if (event_type == MAZE_EVENT_RUN_CANCELLED) {
    status_ = "Run cancelled.";
    return plan;
  }
  if (event_type == MAZE_EVENT_RUN_FAILED) {
    const std::string message = Event::ParseMessage(payload);
    status_ =
        message.empty() ? "Run failed." : std::string("Run failed: ") + message;
    return plan;
  }

  return plan;
}

auto GuiStateStore::GetRasterStateSnapshot() const -> RasterStateSnapshot {
  std::lock_guard<std::mutex> lock(mutex_);
  RasterStateSnapshot snapshot;
  snapshot.maze_width = maze_width_;
  snapshot.maze_height = maze_height_;
  snapshot.unit_pixels = unit_pixels_;
  snapshot.start_x = start_x_;
  snapshot.start_y = start_y_;
  snapshot.end_x = end_x_;
  snapshot.end_y = end_y_;
  snapshot.cell_states = cell_states_;
  snapshot.topology_wall_masks = topology_wall_masks_;
  snapshot.path_cells = path_cells_;
  return snapshot;
}

void GuiStateStore::UpdateRasterFrame(std::vector<unsigned char> bgra, int width,
                                      int height, int channels) {
  std::lock_guard<std::mutex> lock(mutex_);
  latest_bgra_ = std::move(bgra);
  last_width_ = width;
  last_height_ = height;
  last_channels_ = channels;
}

auto GuiStateStore::MutateRasterFrame(
    const std::function<bool(std::vector<unsigned char>&, int, int, int)>& mutator)
    -> bool {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mutator || latest_bgra_.empty() || last_width_ <= 0 || last_height_ <= 0 ||
      last_channels_ <= 0) {
    return false;
  }
  return mutator(latest_bgra_, last_width_, last_height_, last_channels_);
}

auto GuiStateStore::GetPaintSnapshot() const -> PaintSnapshot {
  std::lock_guard<std::mutex> lock(mutex_);
  PaintSnapshot snapshot;
  snapshot.status = status_;
  snapshot.summary_json = summary_json_;
  snapshot.running = running_;
  snapshot.cancel_requested = cancel_requested_;
  snapshot.frame_count = frame_count_;
  snapshot.callback_count = callback_count_;
  snapshot.has_topology_snapshot = has_topology_snapshot_;
  snapshot.last_width = last_width_;
  snapshot.last_height = last_height_;
  snapshot.last_channels = last_channels_;
  snapshot.latest_bgra = latest_bgra_;
  return snapshot;
}

auto GuiStateStore::CellIndexLocked(int row, int col) const -> size_t {
  return static_cast<size_t>(row * maze_width_ + col);
}

void GuiStateStore::ResetSearchSurfaceLocked(const RunParams& params) {
  maze_width_ = params.width;
  maze_height_ = params.height;
  unit_pixels_ = params.unit_pixels;
  start_x_ = params.start_x;
  start_y_ = params.start_y;
  end_x_ = params.end_x;
  end_y_ = params.end_y;
  cell_states_.assign(
      static_cast<size_t>(std::max(0, params.width * params.height)), 0);
  topology_wall_masks_.assign(
      static_cast<size_t>(std::max(0, params.width * params.height)),
      static_cast<unsigned char>(kWallTopBit | kWallRightBit | kWallBottomBit |
                                 kWallLeftBit));
  has_topology_snapshot_ = false;
  path_cells_.clear();
  latest_bgra_.clear();
  last_width_ = 0;
  last_height_ = 0;
  last_channels_ = 0;
}

}  // namespace MazeWinGui::State
