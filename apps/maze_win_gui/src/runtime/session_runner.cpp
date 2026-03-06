#include "src/runtime/session_runner.h"

#include <string_view>
#include <utility>

#include "src/render/maze_rasterizer.h"

namespace MazeWinGui::Runtime {

SessionRunner::SessionRunner(State::GuiStateStore& state_store,
                             std::function<void()> frame_notifier,
                             std::function<void()> run_completed_notifier)
    : state_store_(state_store),
      frame_notifier_(std::move(frame_notifier)),
      run_completed_notifier_(std::move(run_completed_notifier)) {}

SessionRunner::~SessionRunner() {
  RequestCancel();
  if (worker_.joinable()) {
    worker_.join();
  }
}

auto SessionRunner::Start(const State::RunParams& params, std::string request_json,
                          std::string request_label) -> bool {
  if (!state_store_.TryBeginRun(params)) {
    NotifyFrameUpdated();
    return false;
  }

  const auto initial_state = state_store_.GetRasterStateSnapshot();
  auto initial_frame = Render::MazeRasterizer::Rasterize(initial_state);
  state_store_.UpdateRasterFrame(std::move(initial_frame.bgra), initial_frame.width,
                                 initial_frame.height, initial_frame.channels);
  NotifyFrameUpdated();

  if (worker_.joinable()) {
    worker_.join();
  }
  worker_ = std::thread(&SessionRunner::WorkerMain, this, std::move(request_json),
                        std::move(request_label));
  return true;
}

void SessionRunner::RequestCancel() {
  MazeSession* session = nullptr;
  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    session = active_session_;
  }

  if (session == nullptr) {
    state_store_.SetStatus("No active task to cancel.");
    NotifyFrameUpdated();
    return;
  }

  const int cancel_code = MazeSessionCancel(session);
  state_store_.MarkCancelRequestedResult(cancel_code);
  NotifyFrameUpdated();
}

void SessionRunner::OnMazeEvent(const MazeEvent* event, void* user_data) {
  auto* self = static_cast<SessionRunner*>(user_data);
  if (self != nullptr) {
    self->HandleMazeEvent(event);
  }
}

void SessionRunner::HandleMazeEvent(const MazeEvent* event) {
  if (event == nullptr) {
    return;
  }

  std::string_view payload;
  if (event->payload != nullptr && event->payload_size > 0) {
    payload = std::string_view(static_cast<const char*>(event->payload),
                               event->payload_size);
  }
  const auto raster_plan = state_store_.ApplyEvent(event->type, payload);
  if (raster_plan.requires_full_raster) {
    const auto raster_state = state_store_.GetRasterStateSnapshot();
    auto frame = Render::MazeRasterizer::Rasterize(raster_state);
    state_store_.UpdateRasterFrame(std::move(frame.bgra), frame.width, frame.height,
                                   frame.channels);
  } else if (!raster_plan.dirty_cells.empty()) {
    const auto raster_state = state_store_.GetRasterStateSnapshot();
    const bool updated = state_store_.MutateRasterFrame(
        [&](std::vector<unsigned char>& bgra, int width, int height, int channels) {
          return Render::MazeRasterizer::ApplyCellUpdates(
              raster_state, raster_plan.dirty_cells, bgra, width, height, channels);
        });
    if (!updated) {
      auto frame = Render::MazeRasterizer::Rasterize(raster_state);
      state_store_.UpdateRasterFrame(std::move(frame.bgra), frame.width,
                                     frame.height, frame.channels);
    }
  }
  NotifyFrameUpdated();
}

void SessionRunner::WorkerMain(std::string request_json, std::string request_label) {
  MazeSession* session = nullptr;
  if (MazeSessionCreate(&session) != MAZE_STATUS_OK || session == nullptr) {
    state_store_.AbortRun("MazeSessionCreate failed.");
    NotifyRunCompleted();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    active_session_ = session;
  }

  const int callback_code =
      MazeSessionSetEventCallback(session, &SessionRunner::OnMazeEvent, this);
  if (callback_code != MAZE_STATUS_OK) {
    state_store_.AbortRun("MazeSessionSetEventCallback failed.");
    {
      std::lock_guard<std::mutex> lock(session_mutex_);
      active_session_ = nullptr;
    }
    MazeSessionDestroy(session);
    NotifyRunCompleted();
    return;
  }

  state_store_.SetStatus("Running [ " + request_label + " ]");
  NotifyFrameUpdated();

  const int configure_code = MazeSessionConfigure(session, request_json.c_str());
  const int final_run_code =
      (configure_code == MAZE_STATUS_OK) ? MazeSessionRun(session) : configure_code;

  const char* summary = nullptr;
  const char* error = nullptr;
  MazeSessionGetSummaryJson(session, &summary);
  MazeSessionGetLastError(session, &error);
  state_store_.MarkRunCompleted(final_run_code, error, summary);

  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    active_session_ = nullptr;
  }
  MazeSessionDestroy(session);
  NotifyRunCompleted();
}

void SessionRunner::NotifyFrameUpdated() const {
  if (frame_notifier_) {
    frame_notifier_();
  }
}

void SessionRunner::NotifyRunCompleted() const {
  if (run_completed_notifier_) {
    run_completed_notifier_();
  }
}

}  // namespace MazeWinGui::Runtime
