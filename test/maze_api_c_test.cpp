#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "api/maze_api.h"

namespace {

struct CallbackProbe {
  int event_calls = 0;
  int progress_calls = 0;
  int topology_calls = 0;
  int non_empty_payload_calls = 0;
  uint64_t last_seq = 0;
  bool seq_monotonic = true;
  MazeSession* session = nullptr;
  bool request_cancel = false;
  int cancel_after_progress = 0;
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

auto ReadFileText(const std::filesystem::path& path) -> std::string {
  std::ifstream input(path, std::ios::in | std::ios::binary);
  if (!input.is_open()) {
    return {};
  }
  std::ostringstream oss;
  oss << input.rdbuf();
  return oss.str();
}

auto ResolveContractRequestPath() -> std::filesystem::path {
  const auto cwd = std::filesystem::current_path();
  const auto direct = cwd / "tests" / "contracts" / "json_request_smoke_v1.json";
  if (std::filesystem::exists(direct)) {
    return direct;
  }
  const auto parent = cwd.parent_path() / "tests" / "contracts" / "json_request_smoke_v1.json";
  if (std::filesystem::exists(parent)) {
    return parent;
  }
  return direct;
}

void OnEvent(const MazeEvent* event, void* user_data) {
  auto* probe = static_cast<CallbackProbe*>(user_data);
  if (probe == nullptr || event == nullptr) {
    return;
  }
  probe->event_calls += 1;
  if (event->seq <= probe->last_seq) {
    probe->seq_monotonic = false;
  }
  probe->last_seq = event->seq;
  if (event->payload != nullptr && event->payload_size > 0) {
    probe->non_empty_payload_calls += 1;
  }
  if (event->type == MAZE_EVENT_PROGRESS) {
    probe->progress_calls += 1;
  } else if (event->type == MAZE_EVENT_TOPOLOGY_SNAPSHOT) {
    probe->topology_calls += 1;
  }
  if (probe->request_cancel && probe->session != nullptr &&
      probe->progress_calls >= probe->cancel_after_progress) {
    MazeSessionCancel(probe->session);
  }
}

}  // namespace

auto RunMazeApiCTests() -> int {
  int failures = 0;

  MazeSession* session = nullptr;
  const int create_code = MazeSessionCreate(&session);
  ExpectEqual(create_code, static_cast<int>(MAZE_STATUS_OK),
              "MazeSessionCreate should succeed", failures);
  ExpectTrue(session != nullptr, "MazeSessionCreate should return non-null session",
             failures);
  ExpectTrue(std::string(MazeApiVersion()).size() > 0,
             "API version should not be empty", failures);

  CallbackProbe probe;
  probe.session = session;
  const int callback_code = MazeSessionSetEventCallback(session, OnEvent, &probe);
  ExpectEqual(callback_code, static_cast<int>(MAZE_STATUS_OK),
              "MazeSessionSetEventCallback should succeed", failures);

  const char* request_json = R"({
    "schema_version": 1,
    "request": {
      "maze": {
        "width": 8,
        "height": 8,
        "start": {"x": 0, "y": 0},
        "end": {"x": 7, "y": 7}
      },
      "algorithms": {
        "generation": "DFS",
        "search": "BFS"
      },
      "visualization": {
        "unit_pixels": 8
      },
      "stream": {
        "emit_stride": 1,
        "emit_progress": 1
      }
    }
  })";

  const int configure_code = MazeSessionConfigure(session, request_json);
  ExpectEqual(configure_code, static_cast<int>(MAZE_STATUS_OK),
              "MazeSessionConfigure should succeed", failures);
  const int run_code = MazeSessionRun(session);
  ExpectEqual(run_code, static_cast<int>(MAZE_STATUS_OK),
              "MazeSessionRun should succeed for valid request", failures);
  ExpectTrue(probe.event_calls > 0, "Event callback should receive events",
             failures);
  ExpectTrue(probe.progress_calls > 0, "Event callback should receive progress events",
             failures);
  ExpectTrue(probe.topology_calls > 0,
             "Event callback should receive topology snapshot event", failures);
  ExpectTrue(probe.non_empty_payload_calls == probe.event_calls,
             "All events should carry non-empty payloads", failures);
  ExpectTrue(probe.seq_monotonic, "Event seq should increase monotonically",
             failures);

  const char* summary_json = nullptr;
  const int summary_code = MazeSessionGetSummaryJson(session, &summary_json);
  ExpectEqual(summary_code, static_cast<int>(MAZE_STATUS_OK),
              "MazeSessionGetSummaryJson should succeed", failures);
  ExpectTrue(summary_json != nullptr, "Summary json should not be null",
             failures);
  const std::string summary_text = summary_json != nullptr ? summary_json : "";
  ExpectTrue(summary_text.find("\"status\":{\"code\":0") != std::string::npos,
             "Summary json should include success status", failures);

  const int bad_schema_code = MazeSessionConfigure(
      session,
      "{\"schema_version\":2,\"request\":{\"maze\":{\"width\":4,\"height\":4}}}");
  ExpectEqual(bad_schema_code, static_cast<int>(MAZE_STATUS_INVALID_ARGUMENT),
              "Unsupported schema_version should fail", failures);

  const char* error_text = nullptr;
  const int error_code = MazeSessionGetLastError(session, &error_text);
  ExpectEqual(error_code, static_cast<int>(MAZE_STATUS_OK),
              "MazeSessionGetLastError should succeed", failures);
  ExpectTrue(error_text != nullptr &&
                 std::string(error_text).find("Unsupported schema_version") !=
                     std::string::npos,
             "Unsupported schema_version should set clear error", failures);

  const auto contract_request_path = ResolveContractRequestPath();
  const std::string contract_request_text = ReadFileText(contract_request_path);
  ExpectTrue(!contract_request_text.empty(),
             "Contract request JSON should be readable", failures);
  if (!contract_request_text.empty()) {
    const int contract_configure_code =
        MazeSessionConfigure(session, contract_request_text.c_str());
    ExpectEqual(contract_configure_code, static_cast<int>(MAZE_STATUS_OK),
                "Contract request configure should succeed", failures);
    const int contract_run_code = MazeSessionRun(session);
    ExpectEqual(contract_run_code, static_cast<int>(MAZE_STATUS_OK),
                "Contract request should run successfully", failures);
  }

  probe.request_cancel = true;
  probe.cancel_after_progress = 2;
  probe.progress_calls = 0;
  const char* cancellable_request_json = R"({
    "schema_version": 1,
    "request": {
      "maze": {"width": 24, "height": 24},
      "algorithms": {"generation": "DFS", "search": "BFS"},
      "visualization": {"unit_pixels": 2}
    }
  })";
  ExpectEqual(MazeSessionConfigure(session, cancellable_request_json),
              static_cast<int>(MAZE_STATUS_OK),
              "Cancellable request configure should succeed", failures);
  const int cancel_code = MazeSessionRun(session);
  ExpectEqual(cancel_code, static_cast<int>(MAZE_STATUS_CANCELLED),
              "MazeSessionRun should return cancelled when callback requests cancel",
              failures);

  const int cancel_null_code = MazeSessionCancel(nullptr);
  ExpectEqual(cancel_null_code, static_cast<int>(MAZE_STATUS_INVALID_ARGUMENT),
              "MazeSessionCancel should reject null session", failures);

  MazeSessionDestroy(session);
  session = nullptr;

  const int invalid_create_code = MazeSessionCreate(nullptr);
  ExpectEqual(invalid_create_code, static_cast<int>(MAZE_STATUS_INVALID_ARGUMENT),
              "MazeSessionCreate should reject null out pointer", failures);

  return failures;
}
