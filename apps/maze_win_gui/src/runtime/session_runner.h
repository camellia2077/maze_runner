#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "api/maze_api.h"
#include "src/state/gui_state_store.h"

namespace MazeWinGui::Runtime {

class SessionRunner {
 public:
  SessionRunner(State::GuiStateStore& state_store,
                std::function<void()> frame_notifier,
                std::function<void()> run_completed_notifier);
  ~SessionRunner();

  auto Start(const State::RunParams& params, std::string request_json,
             std::string request_label) -> bool;
  void RequestCancel();

 private:
  static void OnMazeEvent(const MazeEvent* event, void* user_data);
  void HandleMazeEvent(const MazeEvent* event);
  void WorkerMain(std::string request_json, std::string request_label);
  void NotifyFrameUpdated() const;
  void NotifyRunCompleted() const;

  State::GuiStateStore& state_store_;
  std::function<void()> frame_notifier_;
  std::function<void()> run_completed_notifier_;
  mutable std::mutex session_mutex_;
  MazeSession* active_session_ = nullptr;
  std::thread worker_;
};

}  // namespace MazeWinGui::Runtime
