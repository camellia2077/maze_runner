#include <iostream>
#include <vector>

#include "domain/maze_solver_event_emitter.h"
#include "domain/maze_solver_grid_utils.h"

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

class CaptureSink final : public MazeSolverDomain::ISearchEventSink {
 public:
  void OnEvent(const MazeSolverDomain::SearchEvent& event) override {
    events.push_back(event);
  }
  auto ShouldCancel() const -> bool override { return cancel; }

  bool cancel = false;
  std::vector<MazeSolverDomain::SearchEvent> events;
};

}  // namespace

auto RunSolverEventEmitterTests() -> int {
  int failures = 0;

  CaptureSink sink;
  MazeSolverDomain::SolveOptions options;
  options.emit_stride = 2;
  options.emit_progress = true;
  auto execution_state = MazeSolverDomain::detail::CreateExecutionState(&sink, options);

  MazeSolverDomain::detail::EmitRunStarted(execution_state);

  const MazeSolverDomain::detail::GridSize grid_size{.height = 2, .width = 2};
  auto visual_states = MazeSolverDomain::detail::CreateStateGrid(
      grid_size, MazeSolverDomain::SolverCellState::NONE);
  visual_states[0][0] = MazeSolverDomain::SolverCellState::FRONTIER;

  MazeSolverDomain::detail::EmitProgress(execution_state, visual_states, {},
                                         false);
  MazeSolverDomain::detail::EmitProgress(execution_state, visual_states, {},
                                         false);
  visual_states[0][1] = MazeSolverDomain::SolverCellState::FRONTIER;
  MazeSolverDomain::detail::EmitProgress(execution_state, visual_states, {},
                                         true);
  MazeSolverDomain::detail::EmitRunFailed(execution_state, "boom");

  ExpectEqual(sink.events.size(), static_cast<size_t>(4),
              "expected run_started + 2 progress + run_failed", failures);
  if (sink.events.size() == 4) {
    ExpectEqual(sink.events[0].type, MazeSolverDomain::SearchEventType::kRunStarted,
                "event 0 type", failures);
    ExpectEqual(sink.events[1].type, MazeSolverDomain::SearchEventType::kProgress,
                "event 1 type", failures);
    ExpectEqual(sink.events[2].type, MazeSolverDomain::SearchEventType::kProgress,
                "event 2 type", failures);
    ExpectEqual(sink.events[3].type, MazeSolverDomain::SearchEventType::kRunFailed,
                "event 3 type", failures);
    ExpectEqual(sink.events[0].seq, static_cast<uint64_t>(1), "seq 1", failures);
    ExpectEqual(sink.events[3].seq, static_cast<uint64_t>(4), "seq 4", failures);
    ExpectTrue(!sink.events[1].deltas.empty(), "first progress should have deltas",
               failures);
  }

  sink.cancel = true;
  ExpectTrue(MazeSolverDomain::detail::ShouldCancel(execution_state),
             "ShouldCancel should forward sink cancel flag", failures);

  return failures;
}
