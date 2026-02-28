#include "config/config.h"
#include "domain/grid_topology.h"

int main() {
  const MazeDomain::GridTopology2D topology(
      MazeDomain::GridSize2D{.height = 2, .width = 3});
  Config::AppConfig config;
  return (topology.CellCount() == 6 && config.maze.width > 0) ? 0 : 1;
}
