#include "api/maze_api.h"

#include <chrono>
#include <cstring>
#include <new>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "application/services/maze_generation.h"
#include "application/services/maze_runtime_context.h"
#include "application/services/maze_solver.h"
#include "config/config.h"
#include "domain/grid_topology.h"
#include "infrastructure/graphics/maze_renderer.h"

struct ApiFrameBuffer {
  int width = 0;
  int height = 0;
  int channels = 0;
  std::vector<unsigned char> pixels;
};

struct MazeHandle {
  Config::AppConfig config;
  std::string last_error;
  std::string summary_json;
  std::vector<ApiFrameBuffer> frame_buffers;
  MazeFrameCallback frame_callback = nullptr;
  void* frame_callback_user_data = nullptr;
  bool has_request = false;
  bool write_png_output = false;
  std::optional<int> random_seed;
  int requested_schema_version = 1;
};

namespace {

constexpr char kApiVersion[] = "0.2.0";
constexpr char kNotImplemented[] = "Not implemented in Step 3.2.";
constexpr int kRgbaChannels = 4;
constexpr int kSupportedSchemaVersion = 1;

auto SetError(MazeHandle* handle, int status_code, std::string message) -> int {
  if (handle != nullptr) {
    handle->last_error = std::move(message);
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

auto ExtractBoolValue(const std::string& json_text, const char* key,
                      bool default_value) -> bool {
  const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(true|false)");
  std::smatch match;
  if (!std::regex_search(json_text, match, pattern)) {
    return default_value;
  }
  return match[1].str() == "true";
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

auto BuildSummaryJson(const Config::AppConfig& config,
                      const std::string& generation_algorithm,
                      const std::string& search_algorithm, int frame_count,
                      bool found, int path_length, long long elapsed_ms,
                      int requested_schema_version, bool write_png,
                      const std::optional<int>& random_seed, int status_code,
                      const std::string& message) -> std::string {
  const std::string safe_message = JsonEscape(message);
  const std::string safe_generation = JsonEscape(generation_algorithm);
  const std::string safe_search = JsonEscape(search_algorithm);
  const std::string safe_output_dir = JsonEscape(config.output_dir);

  std::ostringstream oss;
  oss << "{"
      << "\"schema_version\":" << kSupportedSchemaVersion << ","
      << "\"requested_schema_version\":" << requested_schema_version << ","
      << "\"version\":{"
      << "\"api\":\"" << kApiVersion << "\","
      << "\"schema\":" << kSupportedSchemaVersion << "},"
      << "\"status\":{"
      << "\"code\":" << status_code << ","
      << "\"message\":\"" << safe_message << "\"},"
      << "\"metrics\":{"
      << "\"elapsed_ms\":" << elapsed_ms << ","
      << "\"frames\":" << frame_count << "},"
      << "\"maze\":{"
      << "\"dimension\":" << static_cast<int>(config.maze.dimension) << ","
      << "\"width\":" << config.maze.width << ","
      << "\"height\":" << config.maze.height << "},"
      << "\"algorithms\":{"
      << "\"generation\":\"" << safe_generation << "\","
      << "\"search\":\"" << safe_search << "\"},"
      << "\"path\":{"
      << "\"found\":" << (found ? "true" : "false") << ","
      << "\"length\":" << path_length << ","
      << "\"start\":{"
      << "\"x\":" << config.maze.start_node.second << ","
      << "\"y\":" << config.maze.start_node.first << "},"
      << "\"end\":{"
      << "\"x\":" << config.maze.end_node.second << ","
      << "\"y\":" << config.maze.end_node.first << "}"
      << "},"
      << "\"output\":{"
      << "\"write_png\":" << (write_png ? "true" : "false") << ","
      << "\"output_dir\":\"" << safe_output_dir << "\"},"
      << "\"random_seed\":";
  if (random_seed.has_value()) {
    oss << *random_seed;
  } else {
    oss << "null";
  }
  oss << ",\"error\":{"
      << "\"code\":" << status_code << ","
      << "\"message\":\"" << safe_message << "\"}"
      << "}";
  return oss.str();
}

struct ParsedRequest {
  Config::AppConfig config;
  bool write_png_output = false;
  std::optional<int> random_seed;
  int requested_schema_version = kSupportedSchemaVersion;
};

auto CurrentGenerationAlgorithmName(const Config::AppConfig& config)
    -> std::string {
  if (config.maze.generation_algorithms.empty()) {
    return "";
  }
  return config.maze.generation_algorithms.front().name;
}

auto CurrentSearchAlgorithmName(const Config::AppConfig& config) -> std::string {
  if (config.maze.search_algorithms.empty()) {
    return "";
  }
  return config.maze.search_algorithms.front().name;
}

auto BuildHandleSummary(const MazeHandle* handle, int frame_count, bool found,
                        int path_length, long long elapsed_ms, int status_code,
                        const std::string& message) -> std::string {
  if (handle == nullptr) {
    Config::AppConfig empty_config;
    return BuildSummaryJson(empty_config, "", "", frame_count, found,
                            path_length, elapsed_ms, kSupportedSchemaVersion,
                            false, std::nullopt, status_code, message);
  }
  return BuildSummaryJson(
      handle->config, CurrentGenerationAlgorithmName(handle->config),
      CurrentSearchAlgorithmName(handle->config), frame_count, found, path_length,
      elapsed_ms, handle->requested_schema_version, handle->write_png_output,
      handle->random_seed, status_code, message);
}

auto ConvertToRgba(const MazeSolver::RenderBuffer& input_buffer,
                   ApiFrameBuffer& output_buffer, std::string& error) -> bool {
  if (input_buffer.width <= 0 || input_buffer.height <= 0 ||
      input_buffer.channels <= 0) {
    error = "Invalid input frame buffer dimensions.";
    return false;
  }
  if (input_buffer.pixels.empty()) {
    error = "Input frame buffer is empty.";
    return false;
  }

  const size_t pixel_count = static_cast<size_t>(input_buffer.width) *
                             static_cast<size_t>(input_buffer.height);
  const size_t src_size = pixel_count * static_cast<size_t>(input_buffer.channels);
  if (input_buffer.pixels.size() != src_size) {
    error = "Input frame buffer size mismatch.";
    return false;
  }

  output_buffer.width = input_buffer.width;
  output_buffer.height = input_buffer.height;
  output_buffer.channels = kRgbaChannels;
  output_buffer.pixels.assign(pixel_count * static_cast<size_t>(kRgbaChannels), 0);

  if (input_buffer.channels == kRgbaChannels) {
    output_buffer.pixels = input_buffer.pixels;
    return true;
  }
  if (input_buffer.channels != 3) {
    error = "Unsupported input channels for RGBA conversion.";
    return false;
  }

  for (size_t index = 0; index < pixel_count; ++index) {
    const size_t src = index * 3;
    const size_t dst = index * static_cast<size_t>(kRgbaChannels);
    output_buffer.pixels[dst + 0] = input_buffer.pixels[src + 0];
    output_buffer.pixels[dst + 1] = input_buffer.pixels[src + 1];
    output_buffer.pixels[dst + 2] = input_buffer.pixels[src + 2];
    output_buffer.pixels[dst + 3] = 255;
  }
  return true;
}

auto ParseRequestJson(MazeHandle* handle, const std::string& request_json) -> int {
  if (handle == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  ParsedRequest parsed;
  parsed.requested_schema_version =
      ExtractIntValue(request_json, "schema_version", kSupportedSchemaVersion);
  if (parsed.requested_schema_version <= 0) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "schema_version must be a positive integer.");
  }
  if (parsed.requested_schema_version != kSupportedSchemaVersion) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "Unsupported schema_version: " +
                        std::to_string(parsed.requested_schema_version));
  }

  const std::string request_scope =
      ExtractObjectValue(request_json, "request").value_or(request_json);
  const std::string maze_scope =
      ExtractObjectValue(request_scope, "maze").value_or(request_scope);
  const std::string algorithms_scope =
      ExtractObjectValue(request_scope, "algorithms").value_or(request_scope);
  const std::string visualization_scope =
      ExtractObjectValue(request_scope, "visualization").value_or(request_scope);
  const std::string output_scope =
      ExtractObjectValue(request_scope, "output").value_or(request_scope);
  const std::optional<std::string> start_scope =
      ExtractObjectValue(maze_scope, "start");
  const std::optional<std::string> end_scope = ExtractObjectValue(maze_scope, "end");

  parsed.config.maze.dimension = Config::MazeRuntimeDimension::D2;
  parsed.config.maze.width = ExtractIntValue(maze_scope, "width", 10);
  parsed.config.maze.height = ExtractIntValue(maze_scope, "height", 10);
  parsed.config.maze.unit_pixels =
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
  const int end_default_y = parsed.config.maze.height - 1;
  const int end_default_x = parsed.config.maze.width - 1;
  const int end_y =
      end_scope.has_value()
          ? ExtractIntValue(*end_scope, "y",
                            ExtractIntValue(maze_scope, "end_y", end_default_y))
          : ExtractIntValue(maze_scope, "end_y", end_default_y);
  const int end_x =
      end_scope.has_value()
          ? ExtractIntValue(*end_scope, "x",
                            ExtractIntValue(maze_scope, "end_x", end_default_x))
          : ExtractIntValue(maze_scope, "end_x", end_default_x);
  parsed.config.maze.start_node = {start_y, start_x};
  parsed.config.maze.end_node = {end_y, end_x};
  parsed.config.output_dir = ExtractStringValue(
      output_scope, "output_dir", ExtractStringValue(request_json, "output_dir", "."));
  parsed.write_png_output = ExtractBoolValue(
      output_scope, "write_png", ExtractBoolValue(request_json, "write_png", false));
  parsed.random_seed = ExtractOptionalIntValue(request_scope, "random_seed");

  if (parsed.config.maze.width <= 0 || parsed.config.maze.height <= 0 ||
      parsed.config.maze.unit_pixels <= 0) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "Invalid width/height/unit_pixels in JSON request.");
  }
  if (parsed.config.maze.start_node.first < 0 || parsed.config.maze.start_node.second < 0 ||
      parsed.config.maze.start_node.first >= parsed.config.maze.height ||
      parsed.config.maze.start_node.second >= parsed.config.maze.width) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "Invalid start position in JSON request.");
  }
  if (parsed.config.maze.end_node.first < 0 || parsed.config.maze.end_node.second < 0 ||
      parsed.config.maze.end_node.first >= parsed.config.maze.height ||
      parsed.config.maze.end_node.second >= parsed.config.maze.width) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "Invalid end position in JSON request.");
  }

  const std::string generation_algo_name =
      ExtractStringValue(algorithms_scope, "generation",
                         ExtractStringValue(request_json, "generation_algorithm", "DFS"));
  MazeDomain::MazeAlgorithmType generation_algo = MazeDomain::MazeAlgorithmType::DFS;
  if (!MazeGeneration::try_parse_algorithm(generation_algo_name, generation_algo)) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "Unsupported generation_algorithm: " + generation_algo_name);
  }
  parsed.config.maze.generation_algorithms = {
      {generation_algo, MazeGeneration::algorithm_name(generation_algo)}};

  const std::string solver_algo_name =
      ExtractStringValue(algorithms_scope, "search",
                         ExtractStringValue(request_json, "search_algorithm", "BFS"));
  MazeSolver::SolverAlgorithmType solver_algo = MazeSolver::SolverAlgorithmType::BFS;
  if (!MazeSolver::TryParseAlgorithm(solver_algo_name, solver_algo)) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "Unsupported search_algorithm: " + solver_algo_name);
  }
  parsed.config.maze.search_algorithms = {
      {solver_algo, MazeSolver::AlgorithmName(solver_algo)}};

  handle->config = std::move(parsed.config);
  handle->write_png_output = parsed.write_png_output;
  handle->random_seed = parsed.random_seed;
  handle->requested_schema_version = parsed.requested_schema_version;
  handle->has_request = true;
  return MAZE_STATUS_OK;
}

auto ExecuteCurrentRequest(MazeHandle* handle) -> int {
  if (handle == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  if (!handle->has_request) {
    return SetError(handle, MAZE_STATUS_INVALID_ARGUMENT,
                    "No request loaded. Call MazeRunFromJson first.");
  }

  const auto run_start = std::chrono::steady_clock::now();
  auto& config = handle->config;
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
  const MazeSolver::SearchResult result = MazeSolver::Solve(runtime_context);

  handle->frame_buffers.clear();
  handle->frame_buffers.reserve(result.frames_.size());
  for (const auto& frame : result.frames_) {
    MazeSolver::RenderBuffer rgb_buffer;
    ApiFrameBuffer rgba_buffer;
    std::string render_error;
    if (!MazeSolver::RenderFrameToBuffer(frame, maze_grid, config, rgb_buffer,
                                         render_error)) {
      return SetError(handle, MAZE_STATUS_INTERNAL_ERROR, render_error);
    }
    if (!ConvertToRgba(rgb_buffer, rgba_buffer, render_error)) {
      return SetError(handle, MAZE_STATUS_INTERNAL_ERROR, render_error);
    }
    if (handle->frame_callback != nullptr) {
      handle->frame_callback(rgba_buffer.pixels.data(), rgba_buffer.pixels.size(),
                             rgba_buffer.width, rgba_buffer.height,
                             rgba_buffer.channels,
                             handle->frame_callback_user_data);
    }
    handle->frame_buffers.push_back(std::move(rgba_buffer));
  }

  if (handle->write_png_output) {
    const auto render_result = MazeSolver::RenderSearchResult(
        result, maze_grid, solver_algo,
        MazeGeneration::algorithm_name(generation_algo), config);
    if (!render_result.ok) {
      return SetError(handle, MAZE_STATUS_INTERNAL_ERROR, render_result.error);
    }
  }

  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - run_start)
                              .count();
  handle->summary_json = BuildHandleSummary(
      handle, static_cast<int>(handle->frame_buffers.size()), result.found_,
      static_cast<int>(result.path_.size()), elapsed_ms, MAZE_STATUS_OK, "ok");
  handle->last_error.clear();
  return MAZE_STATUS_OK;
}

}  // namespace

extern "C" {

auto MazeApiVersion(void) -> const char* { return kApiVersion; }

auto MazeCreateHandle(MazeHandle** out_handle) -> int {
  if (out_handle == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  MazeHandle* handle = new (std::nothrow) MazeHandle();
  if (handle == nullptr) {
    return MAZE_STATUS_INTERNAL_ERROR;
  }
  handle->summary_json =
      BuildHandleSummary(handle, 0, false, 0, 0, MAZE_STATUS_OK, "empty");
  *out_handle = handle;
  return MAZE_STATUS_OK;
}

void MazeDestroyHandle(MazeHandle* handle) { delete handle; }

auto MazeGetLastError(const MazeHandle* handle) -> const char* {
  if (handle == nullptr) {
    return "MazeHandle is null.";
  }
  return handle->last_error.c_str();
}

auto MazeGetSummaryJson(const MazeHandle* handle) -> const char* {
  if (handle == nullptr) {
    return "{}";
  }
  return handle->summary_json.c_str();
}

auto MazeSetFrameCallback(MazeHandle* handle, MazeFrameCallback callback,
                          void* user_data) -> int {
  if (handle == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  handle->frame_callback = callback;
  handle->frame_callback_user_data = user_data;
  return MAZE_STATUS_OK;
}

auto MazeRunFromJson(MazeHandle* handle, const char* request_json) -> int {
  if (handle == nullptr || request_json == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  const std::string request_text = request_json;
  handle->requested_schema_version =
      ExtractIntValue(request_text, "schema_version", kSupportedSchemaVersion);
  const int parse_code = ParseRequestJson(handle, request_text);
  if (parse_code != MAZE_STATUS_OK) {
    handle->summary_json =
        BuildHandleSummary(handle, 0, false, 0, 0, parse_code, handle->last_error);
    return parse_code;
  }
  const int run_code = ExecuteCurrentRequest(handle);
  if (run_code != MAZE_STATUS_OK) {
    handle->summary_json = BuildHandleSummary(
        handle, static_cast<int>(handle->frame_buffers.size()), false, 0, 0,
        run_code, handle->last_error);
  }
  return run_code;
}

auto MazeRun(MazeHandle* handle) -> int {
  if (handle == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  if (!handle->has_request) {
    handle->last_error = kNotImplemented;
    return MAZE_STATUS_NOT_IMPLEMENTED;
  }
  const int run_code = ExecuteCurrentRequest(handle);
  if (run_code != MAZE_STATUS_OK) {
    handle->summary_json = BuildHandleSummary(
        handle, static_cast<int>(handle->frame_buffers.size()), false, 0, 0,
        run_code, handle->last_error);
  }
  return run_code;
}

auto MazeGetFrameCount(const MazeHandle* handle, int* out_count) -> int {
  if (handle == nullptr || out_count == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  *out_count = static_cast<int>(handle->frame_buffers.size());
  return MAZE_STATUS_OK;
}

auto MazeGetFrameRgba(const MazeHandle* handle, int frame_index,
                      unsigned char* out_rgba, size_t out_rgba_size,
                      int* out_width, int* out_height) -> int {
  if (handle == nullptr || out_width == nullptr || out_height == nullptr) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  if (frame_index < 0 ||
      frame_index >= static_cast<int>(handle->frame_buffers.size())) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  const auto& buffer = handle->frame_buffers[static_cast<size_t>(frame_index)];
  *out_width = buffer.width;
  *out_height = buffer.height;
  const size_t required_size = buffer.pixels.size();
  if (out_rgba == nullptr || out_rgba_size < required_size) {
    return MAZE_STATUS_INVALID_ARGUMENT;
  }
  std::memcpy(out_rgba, buffer.pixels.data(), required_size);
  return MAZE_STATUS_OK;
}

}  // extern "C"
