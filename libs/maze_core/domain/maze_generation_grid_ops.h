#ifndef MAZE_DOMAIN_MAZE_GENERATION_GRID_OPS_H
#define MAZE_DOMAIN_MAZE_GENERATION_GRID_OPS_H

#include <array>
#include <utility>
#include <vector>

#include "domain/grid_topology.h"
#include "domain/maze_generation.h"

namespace MazeDomain::MazeGenerationDetail {

using Grid = MazeGrid;
using CellPosition = std::pair<int, int>;
using NeighborWithDirection = std::pair<Direction, CellPosition>;

struct DirectionInfo {
  int dr;
  int dc;
  Direction opposite;
};

struct GridSize {
  int width;
  int height;
};

inline constexpr std::array<DirectionInfo, 4> kDirectionTable = {{
    {.dr = -1, .dc = 0, .opposite = Direction::Down},
    {.dr = 0, .dc = 1, .opposite = Direction::Left},
    {.dr = 1, .dc = 0, .opposite = Direction::Up},
    {.dr = 0, .dc = -1, .opposite = Direction::Right},
}};

auto DirectionIndex(Direction dir) -> int;
auto IsWithinBounds(int row, int col, GridSize grid_size) -> bool;
void Carve(Grid& grid, int row, int col, Direction dir);
auto AdjacentWithDirection(const GridTopology2D& topology, int row, int col)
    -> std::vector<NeighborWithDirection>;
auto CreateVisitedGrid(GridSize grid_size) -> std::vector<std::vector<bool>>;
void FillAllWalls(Grid& maze_grid, int width, int height);

}  // namespace MazeDomain::MazeGenerationDetail

#endif  // MAZE_DOMAIN_MAZE_GENERATION_GRID_OPS_H
