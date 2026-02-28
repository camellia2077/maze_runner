#include <iostream>
#include <string>

#include "api/maze_api.h"

int main() {
  MazeHandle* handle = nullptr;
  const int create_code = MazeCreateHandle(&handle);
  if (create_code != MAZE_STATUS_OK || handle == nullptr) {
    std::cerr << "MazeCreateHandle failed: " << create_code << "\n";
    return 1;
  }

  const char* request_json = R"({
    "schema_version": 1,
    "width": 6,
    "height": 6,
    "unit_pixels": 8,
    "generation_algorithm": "DFS",
    "search_algorithm": "BFS",
    "write_png": false
  })";

  const int run_code = MazeRunFromJson(handle, request_json);
  if (run_code != MAZE_STATUS_OK) {
    std::cerr << "MazeRunFromJson failed: " << run_code
              << ", error: " << MazeGetLastError(handle) << "\n";
    MazeDestroyHandle(handle);
    return 1;
  }

  int frame_count = 0;
  const int count_code = MazeGetFrameCount(handle, &frame_count);
  if (count_code != MAZE_STATUS_OK || frame_count <= 0) {
    std::cerr << "MazeGetFrameCount failed: " << count_code
              << ", frame_count=" << frame_count << "\n";
    MazeDestroyHandle(handle);
    return 1;
  }

  const char* summary_json = MazeGetSummaryJson(handle);
  if (summary_json == nullptr || std::string(summary_json).empty()) {
    std::cerr << "MazeGetSummaryJson returned empty result.\n";
    MazeDestroyHandle(handle);
    return 1;
  }

  std::cout << "C API smoke passed. frames=" << frame_count << "\n";
  MazeDestroyHandle(handle);
  return 0;
}
