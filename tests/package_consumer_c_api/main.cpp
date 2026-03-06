#include <iostream>
#include <string>

#include "api/maze_api.h"

int main() {
  MazeSession* session = nullptr;
  const int create_code = MazeSessionCreate(&session);
  if (create_code != MAZE_STATUS_OK || session == nullptr) {
    std::cerr << "MazeSessionCreate failed: " << create_code << "\n";
    return 1;
  }

  const char* request_json = R"({
    "schema_version": 1,
    "request": {
      "maze": {
        "width": 6,
        "height": 6
      },
      "algorithms": {
        "generation": "DFS",
        "search": "BFS"
      },
      "visualization": {
        "unit_pixels": 8
      }
    }
  })";

  const int configure_code = MazeSessionConfigure(session, request_json);
  if (configure_code != MAZE_STATUS_OK) {
    const char* error = nullptr;
    MazeSessionGetLastError(session, &error);
    std::cerr << "MazeSessionConfigure failed: " << configure_code
              << ", error: " << (error != nullptr ? error : "") << "\n";
    MazeSessionDestroy(session);
    return 1;
  }

  const int run_code = MazeSessionRun(session);
  if (run_code != MAZE_STATUS_OK) {
    const char* error = nullptr;
    MazeSessionGetLastError(session, &error);
    std::cerr << "MazeSessionRun failed: " << run_code
              << ", error: " << (error != nullptr ? error : "") << "\n";
    MazeSessionDestroy(session);
    return 1;
  }

  const char* summary_json = nullptr;
  const int summary_code = MazeSessionGetSummaryJson(session, &summary_json);
  if (summary_code != MAZE_STATUS_OK || summary_json == nullptr ||
      std::string(summary_json).empty()) {
    std::cerr << "MazeSessionGetSummaryJson returned empty result.\n";
    MazeSessionDestroy(session);
    return 1;
  }

  std::cout << "C API smoke passed.\n";
  MazeSessionDestroy(session);
  return 0;
}
