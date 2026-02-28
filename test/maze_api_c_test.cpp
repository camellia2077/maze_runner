#include <iostream>
#include <string>
#include <vector>

#include "api/maze_api.h"

namespace {

struct CallbackProbe {
  int frame_calls = 0;
  int last_width = 0;
  int last_height = 0;
  int last_channels = 0;
};

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

void OnFrame(const unsigned char* rgba_data, size_t rgba_size, int width,
             int height, int channels, void* user_data) {
  (void)rgba_data;
  (void)rgba_size;
  auto* probe = static_cast<CallbackProbe*>(user_data);
  if (probe == nullptr) {
    return;
  }
  probe->frame_calls += 1;
  probe->last_width = width;
  probe->last_height = height;
  probe->last_channels = channels;
}

}  // namespace

auto RunMazeApiCTests() -> int {
  int failures = 0;

  MazeHandle* handle = nullptr;
  const int create_code = MazeCreateHandle(&handle);
  ExpectEqual(create_code, static_cast<int>(MAZE_STATUS_OK),
              "CreateHandle should succeed", failures);
  ExpectTrue(handle != nullptr, "CreateHandle should return non-null handle",
             failures);
  ExpectTrue(std::string(MazeApiVersion()).size() > 0,
             "API version should not be empty", failures);

  CallbackProbe probe;
  const int callback_code = MazeSetFrameCallback(handle, OnFrame, &probe);
  ExpectEqual(callback_code, static_cast<int>(MAZE_STATUS_OK),
              "SetFrameCallback should succeed", failures);

  const char* request_json = R"({
    "schema_version": 1,
    "request": {
      "maze": {
        "width": 6,
        "height": 6,
        "start": {"x": 0, "y": 0},
        "end": {"x": 5, "y": 5}
      },
      "algorithms": {
        "generation": "DFS",
        "search": "BFS"
      },
      "visualization": {
        "unit_pixels": 8
      },
      "output": {
        "write_png": false
      },
      "random_seed": 12345
    },
    "ignored_unknown_field": {
      "will_be_ignored": true
    }
  })";
  const int run_json_code = MazeRunFromJson(handle, request_json);
  ExpectEqual(run_json_code, static_cast<int>(MAZE_STATUS_OK),
              "RunFromJson should succeed for valid request", failures);

  int frame_count = -1;
  const int count_code = MazeGetFrameCount(handle, &frame_count);
  ExpectEqual(count_code, static_cast<int>(MAZE_STATUS_OK),
              "GetFrameCount should succeed after run", failures);
  ExpectTrue(frame_count > 0, "Frame count should be positive", failures);
  ExpectEqual(probe.frame_calls, frame_count,
              "Callback frame call count should match frame count", failures);
  ExpectTrue(probe.last_width > 0 && probe.last_height > 0 &&
                 probe.last_channels == 4,
             "Callback should receive valid RGBA frame dimensions/channels",
             failures);

  int frame_width = 0;
  int frame_height = 0;
  const size_t rgba_size = static_cast<size_t>(probe.last_width) *
                           static_cast<size_t>(probe.last_height) *
                           static_cast<size_t>(probe.last_channels);
  std::vector<unsigned char> rgba(rgba_size);
  const int frame_code = MazeGetFrameRgba(handle, 0, rgba.data(), rgba.size(),
                                          &frame_width, &frame_height);
  ExpectEqual(frame_code, static_cast<int>(MAZE_STATUS_OK),
              "GetFrameRgba should succeed for frame 0", failures);
  ExpectEqual(frame_width, probe.last_width,
              "Frame width should match callback width", failures);
  ExpectEqual(frame_height, probe.last_height,
              "Frame height should match callback height", failures);

  const char* summary_json = MazeGetSummaryJson(handle);
  ExpectTrue(summary_json != nullptr, "Summary json should not be null",
             failures);
  const std::string summary_text(summary_json);
  ExpectTrue(summary_text.find("\"status\":{\"code\":0") !=
                 std::string::npos,
             "Summary json should include success status", failures);
  ExpectTrue(summary_text.find("\"metrics\":{\"elapsed_ms\":") !=
                 std::string::npos,
             "Summary json should include elapsed_ms field", failures);
  ExpectTrue(summary_text.find("\"frames\":") != std::string::npos,
             "Summary json should include frames field", failures);
  ExpectTrue(summary_text.find("\"path\":{\"found\":") != std::string::npos,
             "Summary json should include path object", failures);
  ExpectTrue(summary_text.find("\"version\":{\"api\":\"") != std::string::npos,
             "Summary json should include version object", failures);
  ExpectTrue(summary_text.find("\"random_seed\":12345") != std::string::npos,
             "Summary json should include random_seed", failures);

  const char* legacy_request_json = R"({
    "width": 5,
    "height": 4,
    "generation_algorithm": "DFS",
    "search_algorithm": "BFS",
    "write_png": false
  })";
  const int legacy_run_code = MazeRunFromJson(handle, legacy_request_json);
  ExpectEqual(legacy_run_code, static_cast<int>(MAZE_STATUS_OK),
              "Legacy flat request without schema_version should still pass",
              failures);

  const int bad_schema_code =
      MazeRunFromJson(handle, "{\"schema_version\":2,\"request\":{\"maze\":{\"width\":4,\"height\":4}}}");
  ExpectEqual(bad_schema_code, static_cast<int>(MAZE_STATUS_INVALID_ARGUMENT),
              "Unsupported schema_version should fail", failures);
  ExpectTrue(std::string(MazeGetLastError(handle)).find("Unsupported schema_version") !=
                 std::string::npos,
             "Unsupported schema_version should set clear error", failures);

  const int bad_json_code = MazeRunFromJson(handle, "{\"width\":0}");
  ExpectEqual(bad_json_code, static_cast<int>(MAZE_STATUS_INVALID_ARGUMENT),
              "RunFromJson should reject invalid dimensions", failures);
  ExpectTrue(std::string(MazeGetLastError(handle)).find("Invalid") !=
                 std::string::npos,
             "Invalid request should set last error", failures);

  MazeDestroyHandle(handle);
  handle = nullptr;

  const int invalid_create_code = MazeCreateHandle(nullptr);
  ExpectEqual(invalid_create_code, static_cast<int>(MAZE_STATUS_INVALID_ARGUMENT),
              "CreateHandle should reject null out pointer", failures);

  return failures;
}
