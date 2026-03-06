#include "api/maze_api.h"

#include <atomic>
#include <chrono>
#include <new>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

#include "application/services/maze_generation.h"
#include "application/services/maze_runtime_context.h"
#include "application/services/maze_solver.h"
#include "config/config.h"
#include "domain/maze_solver.h"

struct MazeSession {
  Config::AppConfig config;
  std::string last_error;
  std::string summary_json;
  bool configured = false;
  int requested_schema_version = 1;
  std::optional<int> random_seed;
  std::atomic<bool> cancel_requested{false};
  MazeEventCallback event_callback = nullptr;
  void* event_callback_user_data = nullptr;
  std::string event_json_cache;
  MazeSolver::SolveOptions solve_options;
  uint64_t next_event_seq = 1;
};

namespace {

constexpr char kApiVersion[] = "0.3.0";
constexpr int kSupportedSchemaVersion = 1;
constexpr int kWallTop = 0;
constexpr int kWallRight = 1;
constexpr int kWallBottom = 2;
constexpr int kWallLeft = 3;

auto NextEventSeq(MazeSession* session) -> uint64_t {
  if (session == nullptr) {
    return 0;
  }
  const uint64_t seq = session->next_event_seq;
  session->next_event_seq += 1;
  return seq;
}

auto SetError(MazeSession* session, int status_code, std::string message) -> int {
  if (session != nullptr) {
    session->last_error = std::move(message);
  }
  return status_code;
}

auto JsonEscape(const std::string& value) -> std::string {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char ch : value) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

auto ExtractIntValue(const std::string& json_text, const char* key,
                     int default_value) -> int {
  const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(-?\\d+)");
  std::smatch match;
  if (!std::regex_search(json_text, match, pattern)) {
    return default_value;
  }
  return std::stoi(match[1].str());
}

auto ExtractOptionalIntValue(const std::string& json_text, const char* key)
    -> std::optional<int> {
  const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(-?\\d+)");
  std::smatch match;
  if (!std::regex_search(json_text, match, pattern)) {
    return std::nullopt;
  }
  return std::stoi(match[1].str());
}

auto ExtractStringValue(const std::string& json_text, const char* key,
                        const std::string& default_value) -> std::string {
  const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*\"([^\"]*)\"");
  std::smatch match;
  if (!std::regex_search(json_text, match, pattern)) {
    return default_value;
  }
  return match[1].str();
}

auto ExtractObjectValue(const std::string& json_text, const char* key)
    -> std::optional<std::string> {
  const std::regex key_pattern("\"" + std::string(key) + "\"\\s*:");
  std::smatch key_match;
  if (!std::regex_search(json_text, key_match, key_pattern)) {
    return std::nullopt;
  }
  const size_t key_pos = static_cast<size_t>(key_match.position(0));
  size_t start = json_text.find('{', key_pos);
  if (start == std::string::npos) {
    return std::nullopt;
  }

  bool in_string = false;
  bool escaped = false;
  int depth = 0;
  for (size_t idx = start; idx < json_text.size(); ++idx) {
    const char ch = json_text[idx];
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (ch == '\\') {
        escaped = true;
      } else if (ch == '"') {
        in_string = false;
      }
      continue;
    }

    if (ch == '"') {
      in_string = true;
      continue;
    }
    if (ch == '{') {
      ++depth;
      continue;
    }
    if (ch == '}') {
      --depth;
      if (depth == 0) {
        return json_text.substr(start, idx - start + 1);
      }
    }
  }
  return std::nullopt;
}

auto BuildSummaryJson(const MazeSession* session, const std::string& generation,
                      const std::string& search, bool found, int path_length,
                      long long elapsed_ms, int status_code,
                      const std::string& message) -> std::string {
  Config::AppConfig config;
  int requested_schema_version = kSupportedSchemaVersion;
  std::optional<int> random_seed;
  int emit_stride = 1;
  if (session != nullptr) {
    config = session->config;
    requested_schema_version = session->requested_schema_version;
    random_seed = session->random_seed;
    emit_stride = session->solve_options.emit_stride;
  }

  std::ostringstream oss;
  oss << "{"
      << "\"schema_version\":" << kSupportedSchemaVersion << ","
      << "\"requested_schema_version\":" << requested_schema_version << ","
      << "\"version\":{"
      << "\"api\":\"" << kApiVersion << "\","
      << "\"schema\":" << kSupportedSchemaVersion << "},"
      << "\"status\":{"
      << "\"code\":" << status_code << ","
      << "\"message\":\"" << JsonEscape(message) << "\"},"
      << "\"metrics\":{"
      << "\"elapsed_ms\":" << elapsed_ms << ","
      << "\"emit_stride\":" << emit_stride << "},"
      << "\"maze\":{"
      << "\"dimension\":" << static_cast<int>(config.maze.dimension) << ","
      << "\"width\":" << config.maze.width << ","
      << "\"height\":" << config.maze.height << "},"
      << "\"algorithms\":{"
      << "\"generation\":\"" << JsonEscape(generation) << "\","
      << "\"search\":\"" << JsonEscape(search) << "\"},"
      << "\"path\":{"
      << "\"found\":" << (found ? "true" : "false") << ","
      << "\"length\":" << path_length << "},"
      << "\"random_seed\":";
  if (random_seed.has_value()) {
    oss << *random_seed;
  } else {
    oss << "null";
  }
  oss << "}";
  return oss.str();
}

auto MapEventType(MazeSolverDomain::SearchEventType event_type) -> int {
  using SearchEventType = MazeSolverDomain::SearchEventType;
  switch (event_type) {
    case SearchEventType::kRunStarted:
      return MAZE_EVENT_RUN_STARTED;
    case SearchEventType::kCellStateChanged:
      return MAZE_EVENT_CELL_STATE_CHANGED;
    case SearchEventType::kPathUpdated:
      return MAZE_EVENT_PATH_UPDATED;
    case SearchEventType::kProgress:
      return MAZE_EVENT_PROGRESS;
    case SearchEventType::kRunFinished:
      return MAZE_EVENT_RUN_FINISHED;
    case SearchEventType::kRunCancelled:
      return MAZE_EVENT_RUN_CANCELLED;
    case SearchEventType::kRunFailed:
      return MAZE_EVENT_RUN_FAILED;
  }
  return MAZE_EVENT_PROGRESS;
}

auto EncodeWallMask(const MazeDomain::MazeCell& cell) -> int {
  int mask = 0;
  if (cell.walls[kWallTop]) {
    mask |= (1 << kWallTop);
  }
  if (cell.walls[kWallRight]) {
    mask |= (1 << kWallRight);
  }
  if (cell.walls[kWallBottom]) {
    mask |= (1 << kWallBottom);
  }
  if (cell.walls[kWallLeft]) {
    mask |= (1 << kWallLeft);
  }
  return mask;
}

auto BuildTopologySnapshotJson(const MazeSession* session,
                               const MazeDomain::MazeGrid& maze_grid,
                               uint64_t seq) -> std::string {
  const int height = static_cast<int>(maze_grid.size());
  const int width = height > 0 ? static_cast<int>(maze_grid.front().size()) : 0;
  const auto start =
      (session != nullptr) ? session->config.maze.start_node
                           : MazeSolverDomain::GridPosition{0, 0};
  const auto end =
      (session != nullptr) ? session->config.maze.end_node
                           : MazeSolverDomain::GridPosition{0, 0};

  std::ostringstream oss;
  oss << "{"
      << "\"seq\":" << seq << ","
      << "\"type\":" << MAZE_EVENT_TOPOLOGY_SNAPSHOT << ","
      << "\"width\":" << width << ","
      << "\"height\":" << height << ","
      << "\"start\":{\"row\":" << start.first << ",\"col\":" << start.second
      << "},"
      << "\"end\":{\"row\":" << end.first << ",\"col\":" << end.second << "},"
      << "\"wall_mask_order\":\"TRBL\","
      << "\"walls\":[";
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      if (row > 0 || col > 0) {
        oss << ",";
      }
      oss << EncodeWallMask(maze_grid[row][col]);
    }
  }
  oss << "]}";
  return oss.str();
}

auto BuildEventJson(const MazeSolverDomain::SearchEvent& event, uint64_t seq)
    -> std::string {
  std::ostringstream oss;
  oss << "{"
      << "\"seq\":" << seq << ","
      << "\"type\":" << MapEventType(event.type) << ","
      << "\"visited_count\":" << event.visited_count << ","
      << "\"frontier_count\":" << event.frontier_count << ","
      << "\"found\":" << (event.found ? "true" : "false") << ","
      << "\"message\":\"" << JsonEscape(event.message) << "\","
      << "\"path\":[";
  for (size_t i = 0; i < event.path.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << "{\"row\":" << event.path[i].first
        << ",\"col\":" << event.path[i].second << "}";
  }
  oss << "],\"deltas\":[";
  for (size_t i = 0; i < event.deltas.size(); ++i) {
    if (i > 0) {
      oss << ",";
    }
    oss << "{\"row\":" << event.deltas[i].row
        << ",\"col\":" << event.deltas[i].col
        << ",\"state\":" << static_cast<int>(event.deltas[i].state) << "}";
  }
  oss << "]}";
  return oss.str();
}

void EmitTopologySnapshot(MazeSession* session,
                          const MazeDomain::MazeGrid& maze_grid) {
  if (session == nullptr || session->event_callback == nullptr) {
    return;
  }
  const uint64_t seq = NextEventSeq(session);
  session->event_json_cache =
      BuildTopologySnapshotJson(session, maze_grid, seq);
  MazeEvent event{
      .seq = seq,
      .type = MAZE_EVENT_TOPOLOGY_SNAPSHOT,
      .payload = session->event_json_cache.data(),
      .payload_size = session->event_json_cache.size()};
  session->event_callback(&event, session->event_callback_user_data);
}

class SessionEventSink final : public MazeSolverDomain::ISearchEventSink {
 public:
  explicit SessionEventSink(MazeSession* session) : session_(session) {}

  void OnEvent(const MazeSolverDomain::SearchEvent& event) override {
    if (session_ == nullptr || session_->event_callback == nullptr) {
      return;
    }
    const uint64_t seq = NextEventSeq(session_);
    session_->event_json_cache = BuildEventJson(event, seq);
    MazeEvent c_event{
        .seq = seq,
        .type = MapEventType(event.type),
        .payload = session_->event_json_cache.data(),
        .payload_size = session_->event_json_cache.size()};
    session_->event_callback(&c_event, session_->event_callback_user_data);
  }

  auto ShouldCancel() const -> bool override {
    if (session_ == nullptr) {
      return false;
    }
    return session_->cancel_requested.load();
  }

 private:
  MazeSession* session_ = nullptr;
};

auto ParseRequestJson(MazeSession* session, const std::string& request_json)
    -> int {
  if (session == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }

  session->requested_schema_version =
      ExtractIntValue(request_json, "schema_version", kSupportedSchemaVersion);
  if (session->requested_schema_version <= 0) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "schema_version must be a positive integer.");
  }
  if (session->requested_schema_version != kSupportedSchemaVersion) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Unsupported schema_version: " +
                        std::to_string(session->requested_schema_version));
  }

  const std::string request_scope =
      ExtractObjectValue(request_json, "request").value_or(request_json);
  const std::string maze_scope =
      ExtractObjectValue(request_scope, "maze").value_or(request_scope);
  const std::string algorithms_scope =
      ExtractObjectValue(request_scope, "algorithms").value_or(request_scope);
  const std::string visualization_scope =
      ExtractObjectValue(request_scope, "visualization").value_or(request_scope);
  const std::string stream_scope =
      ExtractObjectValue(request_scope, "stream").value_or(std::string{});
  const std::optional<std::string> start_scope =
      ExtractObjectValue(maze_scope, "start");
  const std::optional<std::string> end_scope = ExtractObjectValue(maze_scope, "end");

  Config::AppConfig config;
  config.maze.dimension = Config::MazeRuntimeDimension::D2;
  config.maze.width = ExtractIntValue(maze_scope, "width", 10);
  config.maze.height = ExtractIntValue(maze_scope, "height", 10);
  config.maze.unit_pixels =
      ExtractIntValue(visualization_scope, "unit_pixels",
                      ExtractIntValue(maze_scope, "unit_pixels", 12));

  const int start_y = start_scope.has_value()
                          ? ExtractIntValue(*start_scope, "y",
                                            ExtractIntValue(maze_scope, "start_y", 0))
                          : ExtractIntValue(maze_scope, "start_y", 0);
  const int start_x = start_scope.has_value()
                          ? ExtractIntValue(*start_scope, "x",
                                            ExtractIntValue(maze_scope, "start_x", 0))
                          : ExtractIntValue(maze_scope, "start_x", 0);
  const int end_default_y = config.maze.height - 1;
  const int end_default_x = config.maze.width - 1;
  const int end_y = end_scope.has_value()
                        ? ExtractIntValue(*end_scope, "y",
                                          ExtractIntValue(maze_scope, "end_y", end_default_y))
                        : ExtractIntValue(maze_scope, "end_y", end_default_y);
  const int end_x = end_scope.has_value()
                        ? ExtractIntValue(*end_scope, "x",
                                          ExtractIntValue(maze_scope, "end_x", end_default_x))
                        : ExtractIntValue(maze_scope, "end_x", end_default_x);
  config.maze.start_node = {start_y, start_x};
  config.maze.end_node = {end_y, end_x};

  if (config.maze.width <= 0 || config.maze.height <= 0 ||
      config.maze.unit_pixels <= 0) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Invalid width/height/unit_pixels in request.");
  }
  if (config.maze.start_node.first < 0 || config.maze.start_node.second < 0 ||
      config.maze.start_node.first >= config.maze.height ||
      config.maze.start_node.second >= config.maze.width) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Invalid start position in request.");
  }
  if (config.maze.end_node.first < 0 || config.maze.end_node.second < 0 ||
      config.maze.end_node.first >= config.maze.height ||
      config.maze.end_node.second >= config.maze.width) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Invalid end position in request.");
  }

  const std::string generation_algo_name =
      ExtractStringValue(algorithms_scope, "generation", "DFS");
  MazeDomain::MazeAlgorithmType generation_algo = MazeDomain::MazeAlgorithmType::DFS;
  if (!MazeGeneration::try_parse_algorithm(generation_algo_name, generation_algo)) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Unsupported generation algorithm: " + generation_algo_name);
  }
  config.maze.generation_algorithms = {
      {generation_algo, MazeGeneration::algorithm_name(generation_algo)}};

  const std::string solver_algo_name =
      ExtractStringValue(algorithms_scope, "search", "BFS");
  MazeSolver::SolverAlgorithmType solver_algo = MazeSolver::SolverAlgorithmType::BFS;
  if (!MazeSolver::TryParseAlgorithm(solver_algo_name, solver_algo)) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Unsupported search algorithm: " + solver_algo_name);
  }
  config.maze.search_algorithms = {
      {solver_algo, MazeSolver::AlgorithmName(solver_algo)}};

  session->solve_options.emit_stride =
      stream_scope.empty() ? 1 : ExtractIntValue(stream_scope, "emit_stride", 1);
  if (session->solve_options.emit_stride <= 0) {
    session->solve_options.emit_stride = 1;
  }
  session->solve_options.emit_progress =
      ExtractIntValue(stream_scope, "emit_progress", 1) != 0;

  session->random_seed = ExtractOptionalIntValue(request_scope, "random_seed");
  session->config = std::move(config);
  session->configured = true;
  session->last_error.clear();
  return MAZE_STATUS_OK;
}

}  // namespace

extern "C" {

auto MazeApiVersion(void) -> const char* { return kApiVersion; }

auto MazeSessionCreate(MazeSession** out_session) -> int {
  if (out_session == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  MazeSession* session = new (std::nothrow) MazeSession();
  if (session == nullptr) {
    return MAZE_STATUS_INTERNAL_ERROR;
  }
  session->summary_json = BuildSummaryJson(
      session, "", "", false, 0, 0, MAZE_STATUS_OK, "empty");
  *out_session = session;
  return MAZE_STATUS_OK;
}

void MazeSessionDestroy(MazeSession* session) { delete session; }

auto MazeSessionConfigure(MazeSession* session, const char* request_json) -> int {
  if (session == nullptr || request_json == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  return ParseRequestJson(session, request_json);
}

auto MazeSessionSetEventCallback(MazeSession* session,
                                 MazeEventCallback callback,
                                 void* user_data) -> int {
  if (session == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  session->event_callback = callback;
  session->event_callback_user_data = user_data;
  return MAZE_STATUS_OK;
}

auto MazeSessionRun(MazeSession* session) -> int {
  if (session == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  if (!session->configured) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Session is not configured. Call MazeSessionConfigure first.");
  }

  const auto run_start = std::chrono::steady_clock::now();
  session->cancel_requested.store(false);
  session->next_event_seq = 1;

  if (session->config.maze.generation_algorithms.empty() ||
      session->config.maze.search_algorithms.empty()) {
    return SetError(session, MAZE_STATUS_INVALID_ARGUMENT,
                    "Missing generation or search algorithm.");
  }

  auto& config = session->config;
  auto maze_grid = MazeDomain::MazeGrid(
      static_cast<size_t>(config.maze.height),
      MazeDomain::MazeGrid::value_type(static_cast<size_t>(config.maze.width)));
  const MazeDomain::GridTopology2D topology(
      MazeDomain::GridSize2D{.height = config.maze.height,
                             .width = config.maze.width});

  const auto generation_algo = config.maze.generation_algorithms.front().type;
  const auto solver_algo = config.maze.search_algorithms.front().type;
  auto runtime_context = MazeApplication::BuildRuntimeContext2D(
      maze_grid, topology, generation_algo, solver_algo, config);
  runtime_context.start_node_ = config.maze.start_node;
  MazeGeneration::generate_maze_structure(runtime_context);
  EmitTopologySnapshot(session, maze_grid);

  SessionEventSink sink(session);
  const MazeSolver::SearchResult result =
      MazeSolver::Solve(runtime_context, &sink, session->solve_options);

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - run_start)
                              .count();

  if (result.cancelled_) {
    session->last_error = "Cancelled by caller.";
    session->summary_json = BuildSummaryJson(
        session, MazeGeneration::algorithm_name(generation_algo),
        MazeSolver::AlgorithmName(solver_algo), false, 0, elapsed_ms,
        MAZE_STATUS_CANCELLED, session->last_error);
    return MAZE_STATUS_CANCELLED;
  }

  session->last_error.clear();
  session->summary_json = BuildSummaryJson(
      session, MazeGeneration::algorithm_name(generation_algo),
      MazeSolver::AlgorithmName(solver_algo), result.found_,
      static_cast<int>(result.path_.size()), elapsed_ms, MAZE_STATUS_OK, "ok");
  return MAZE_STATUS_OK;
}

auto MazeSessionCancel(MazeSession* session) -> int {
  if (session == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  session->cancel_requested.store(true);
  return MAZE_STATUS_OK;
}

auto MazeSessionGetSummaryJson(const MazeSession* session,
                               const char** out_json) -> int {
  if (session == nullptr || out_json == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  *out_json = session->summary_json.c_str();
  return MAZE_STATUS_OK;
}

auto MazeSessionGetLastError(const MazeSession* session,
                             const char** out_error) -> int {
  if (session == nullptr || out_error == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  *out_error = session->last_error.c_str();
  return MAZE_STATUS_OK;
}

}  // extern "C"
