#include <iostream>
#include <vector>

#include "domain/maze_solver_event_emitter.h"
#include "domain/maze_solver_grid_utils.h"
#include "domain/maze_solver_result_builder.h"

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
  auto ShouldCancel() const -> bool override { return false; }

  std::vector<MazeSolverDomain::SearchEvent> events;
};

}  // namespace

auto RunSolverResultBuilderTests() -> int {
  int failures = 0;

  const MazeSolverDomain::detail::GridSize grid_size{.height = 2, .width = 2};

  {
    CaptureSink sink;
    auto execution_state =
        MazeSolverDomain::detail::CreateExecutionState(&sink, {});
    const auto result = MazeSolverDomain::detail::CreateTrivialResult(
        grid_size, {1, 1}, execution_state);
    ExpectTrue(result.found_, "trivial result should be found", failures);
    ExpectEqual(result.path_.size(), static_cast<size_t>(1),
                "trivial path size", failures);
    if (!result.path_.empty()) {
      ExpectEqual(result.path_.front(), MazeSolverDomain::GridPosition({1, 1}),
                  "trivial path node", failures);
    }
    ExpectTrue(!sink.events.empty(), "trivial result should emit events", failures);
    if (!sink.events.empty()) {
      ExpectEqual(sink.events.back().type,
                  MazeSolverDomain::SearchEventType::kRunFinished,
                  "trivial last event type", failures);
    }
  }

  {
    CaptureSink sink;
    auto execution_state =
        MazeSolverDomain::detail::CreateExecutionState(&sink, {});
    MazeSolverDomain::SearchResult result;
    auto visual_states = MazeSolverDomain::detail::CreateStateGrid(
        grid_size, MazeSolverDomain::SolverCellState::VISITED_PROC);
    auto visited = MazeSolverDomain::detail::CreateBoolGrid(grid_size, false);
    visited[0][0] = true;
    visited[0][1] = true;
    auto parents = MazeSolverDomain::detail::CreateParentGrid(
        grid_size, MazeSolverDomain::detail::kInvalidCellId);
    parents[0][1] =
        MazeSolverDomain::detail::ToCellId(grid_size, MazeSolverDomain::GridPosition({0, 0}));

    const MazeSolverDomain::detail::PathEndpoints endpoints{
        .start = {0, 0}, .end = {0, 1}};
    MazeSolverDomain::detail::FinalizeSearchResult(
        true, endpoints, parents, visual_states, std::move(visited),
        execution_state, result);

    ExpectTrue(result.found_, "finalized result should be found", failures);
    ExpectEqual(result.path_.size(), static_cast<size_t>(2),
                "finalized path should have two nodes", failures);
    if (result.path_.size() == 2) {
      ExpectEqual(result.path_.front(), MazeSolverDomain::GridPosition({0, 0}),
                  "path start", failures);
      ExpectEqual(result.path_.back(), MazeSolverDomain::GridPosition({0, 1}),
                  "path end", failures);
    }

    bool has_path_updated = false;
    for (const auto& event : sink.events) {
      if (event.type == MazeSolverDomain::SearchEventType::kPathUpdated) {
        has_path_updated = true;
      }
    }
    ExpectTrue(has_path_updated, "FinalizeSearchResult should emit path event",
               failures);
    if (!sink.events.empty()) {
      ExpectEqual(sink.events.back().type,
                  MazeSolverDomain::SearchEventType::kRunFinished,
                  "final event should be run finished", failures);
    }
  }

  return failures;
}
