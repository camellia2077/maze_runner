#include <iostream>
#include <string>
#include <vector>

#include "src/event/maze_event_parser.h"

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

auto RunWinGuiParserTests() -> int {
  int failures = 0;

  {
    const std::string payload =
        R"({"deltas":[{"row":0,"col":1,"state":4},{"row":1,"col":1,"state":6}]})";
    const auto deltas = MazeWinGui::Event::ParseDeltas(payload);
    ExpectEqual(deltas.size(), static_cast<size_t>(2),
                "ParseDeltas should return two entries", failures);
    if (deltas.size() == 2) {
      ExpectEqual(deltas[0].row, 0, "delta[0].row", failures);
      ExpectEqual(deltas[0].col, 1, "delta[0].col", failures);
      ExpectEqual(deltas[0].state, 4, "delta[0].state", failures);
      ExpectEqual(deltas[1].state, 6, "delta[1].state", failures);
    }
  }

  {
    const std::string payload =
        R"({"path":[{"row":0,"col":0},{"row":0,"col":1},{"row":1,"col":1}]})";
    const auto path = MazeWinGui::Event::ParsePath(payload);
    ExpectEqual(path.size(), static_cast<size_t>(3),
                "ParsePath should return path entries", failures);
    if (path.size() == 3) {
      ExpectEqual(path.front().row, 0, "path.front.row", failures);
      ExpectEqual(path.back().col, 1, "path.back.col", failures);
    }
  }

  {
    const std::string payload = R"({"message":"run failed for test"})";
    const std::string message = MazeWinGui::Event::ParseMessage(payload);
    ExpectEqual(message, std::string("run failed for test"),
                "ParseMessage should parse message field", failures);
  }

  {
    const std::string payload =
        R"({"width":2,"height":2,"start":{"row":0,"col":0},"end":{"row":1,"col":1},"walls":[15,14,13,12]})";
    MazeWinGui::Event::TopologySnapshot snapshot;
    const bool ok = MazeWinGui::Event::ParseTopologySnapshot(payload, snapshot);
    ExpectTrue(ok, "ParseTopologySnapshot should parse valid payload", failures);
    if (ok) {
      ExpectEqual(snapshot.width, 2, "snapshot.width", failures);
      ExpectEqual(snapshot.height, 2, "snapshot.height", failures);
      ExpectEqual(snapshot.wall_masks.size(), static_cast<size_t>(4),
                  "snapshot wall size", failures);
      ExpectEqual(static_cast<int>(snapshot.wall_masks[1]), 14,
                  "snapshot wall value", failures);
    }
  }

  {
    const std::string payload =
        R"({"width":2,"height":2,"start":{"row":0,"col":0},"end":{"row":1,"col":1},"walls":[15,14,13]})";
    MazeWinGui::Event::TopologySnapshot snapshot;
    const bool ok = MazeWinGui::Event::ParseTopologySnapshot(payload, snapshot);
    ExpectTrue(!ok, "ParseTopologySnapshot should reject wrong wall count",
               failures);
  }

  return failures;
}
