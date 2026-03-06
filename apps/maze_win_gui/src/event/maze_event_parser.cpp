#include "src/event/maze_event_parser.h"

#include <algorithm>
#include <regex>
#include <string>

namespace MazeWinGui::Event {
namespace {

auto ExtractArrayBody(const std::string& json_text, const char* key)
    -> std::string {
  const std::string token = "\"" + std::string(key) + "\":[";
  const size_t token_pos = json_text.find(token);
  if (token_pos == std::string::npos) {
    return {};
  }
  const size_t start = token_pos + token.size() - 1;

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
    if (ch == '[') {
      ++depth;
      continue;
    }
    if (ch == ']') {
      --depth;
      if (depth == 0) {
        return json_text.substr(start + 1, idx - start - 1);
      }
    }
  }
  return {};
}

auto ParseIntField(const std::string& payload, const char* key, int& value)
    -> bool {
  const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(-?\\d+)");
  std::smatch match;
  if (!std::regex_search(payload, match, pattern)) {
    return false;
  }
  value = std::stoi(match[1].str());
  return true;
}

auto ParseCoordField(const std::string& payload, const char* key, CellCoord& coord)
    -> bool {
  const std::regex pattern(
      "\"" + std::string(key) +
      "\"\\s*:\\s*\\{\\s*\"row\"\\s*:\\s*(-?\\d+)\\s*,\\s*\"col\"\\s*:\\s*(-?\\d+)\\s*\\}");
  std::smatch match;
  if (!std::regex_search(payload, match, pattern)) {
    return false;
  }
  coord.row = std::stoi(match[1].str());
  coord.col = std::stoi(match[2].str());
  return true;
}

auto ParseTopologyWallMasks(const std::string& payload)
    -> std::vector<unsigned char> {
  std::vector<unsigned char> wall_masks;
  const std::string body = ExtractArrayBody(payload, "walls");
  if (body.empty()) {
    return wall_masks;
  }

  static const std::regex number_pattern(R"((-?\d+))");
  for (std::sregex_iterator it(body.begin(), body.end(), number_pattern), end;
       it != end; ++it) {
    int value = std::stoi((*it)[1].str());
    value = std::max(0, std::min(15, value));
    wall_masks.push_back(static_cast<unsigned char>(value));
  }
  return wall_masks;
}

}  // namespace

auto ParseDeltas(std::string_view payload_view) -> std::vector<CellDelta> {
  std::vector<CellDelta> deltas;
  const std::string payload(payload_view);
  const std::string body = ExtractArrayBody(payload, "deltas");
  if (body.empty()) {
    return deltas;
  }
  static const std::regex pattern(
      R"(\{"row":(-?\d+),"col":(-?\d+),"state":(-?\d+)\})");
  for (std::sregex_iterator it(body.begin(), body.end(), pattern), end;
       it != end; ++it) {
    deltas.push_back({.row = std::stoi((*it)[1].str()),
                      .col = std::stoi((*it)[2].str()),
                      .state = std::stoi((*it)[3].str())});
  }
  return deltas;
}

auto ParsePath(std::string_view payload_view) -> std::vector<CellCoord> {
  std::vector<CellCoord> path;
  const std::string payload(payload_view);
  const std::string body = ExtractArrayBody(payload, "path");
  if (body.empty()) {
    return path;
  }
  static const std::regex pattern(R"(\{"row":(-?\d+),"col":(-?\d+)\})");
  for (std::sregex_iterator it(body.begin(), body.end(), pattern), end;
       it != end; ++it) {
    path.push_back({.row = std::stoi((*it)[1].str()),
                    .col = std::stoi((*it)[2].str())});
  }
  return path;
}

auto ParseMessage(std::string_view payload_view) -> std::string {
  static const std::regex pattern(R"msg("message":"([^"]*)")msg");
  const std::string payload(payload_view);
  std::smatch match;
  if (!std::regex_search(payload, match, pattern)) {
    return {};
  }
  return match[1].str();
}

auto ParseTopologySnapshot(std::string_view payload_view,
                           TopologySnapshot& snapshot) -> bool {
  const std::string payload(payload_view);
  if (!ParseIntField(payload, "width", snapshot.width) ||
      !ParseIntField(payload, "height", snapshot.height)) {
    return false;
  }
  if (snapshot.width <= 0 || snapshot.height <= 0) {
    return false;
  }
  if (!ParseCoordField(payload, "start", snapshot.start) ||
      !ParseCoordField(payload, "end", snapshot.end)) {
    return false;
  }
  snapshot.wall_masks = ParseTopologyWallMasks(payload);
  const size_t expected =
      static_cast<size_t>(snapshot.width) * static_cast<size_t>(snapshot.height);
  return snapshot.wall_masks.size() == expected;
}

}  // namespace MazeWinGui::Event
