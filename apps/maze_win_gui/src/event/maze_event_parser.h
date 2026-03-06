#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace MazeWinGui::Event {

struct CellCoord {
  int row = 0;
  int col = 0;
};

struct CellDelta {
  int row = 0;
  int col = 0;
  int state = 0;
};

struct TopologySnapshot {
  int width = 0;
  int height = 0;
  CellCoord start;
  CellCoord end;
  std::vector<unsigned char> wall_masks;
};

auto ParseDeltas(std::string_view payload) -> std::vector<CellDelta>;
auto ParsePath(std::string_view payload) -> std::vector<CellCoord>;
auto ParseMessage(std::string_view payload) -> std::string;
auto ParseTopologySnapshot(std::string_view payload, TopologySnapshot& snapshot)
    -> bool;

}  // namespace MazeWinGui::Event
