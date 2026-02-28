#include "domain/maze_generation.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <numeric>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "domain/grid_topology.h"

namespace {

using Grid = MazeDomain::MazeGrid;
using Direction = MazeDomain::Direction;
using CellPosition = std::pair<int, int>;
using Topology2D = MazeDomain::GridTopology2D;

struct DirectionInfo {
  int dr;
  int dc;
  Direction opposite;
};

struct GridSize {
  int width;
  int height;
};

constexpr int kMinDivisionSpan = 2;

constexpr std::array<DirectionInfo, 4> kDirectionTable = {{
    {.dr = -1, .dc = 0, .opposite = Direction::Down},   // Up
    {.dr = 0, .dc = 1, .opposite = Direction::Left},    // Right
    {.dr = 1, .dc = 0, .opposite = Direction::Up},      // Down
    {.dr = 0, .dc = -1, .opposite = Direction::Right},  // Left
}};

constexpr std::array<Direction, 4> kAllDirections = {
    Direction::Up, Direction::Right, Direction::Down, Direction::Left};

auto DirectionIndex(Direction dir) -> int {
  return static_cast<int>(dir);
}

auto TryDirectionFromTo(CellPosition from, CellPosition to)
    -> std::optional<Direction> {
  const int kRowDelta = to.first - from.first;
  const int kColDelta = to.second - from.second;
  if (kRowDelta == -1 && kColDelta == 0) {
    return Direction::Up;
  }
  if (kRowDelta == 0 && kColDelta == 1) {
    return Direction::Right;
  }
  if (kRowDelta == 1 && kColDelta == 0) {
    return Direction::Down;
  }
  if (kRowDelta == 0 && kColDelta == -1) {
    return Direction::Left;
  }
  return std::nullopt;
}

auto AdjacentWithDirection(const Topology2D& topology, int row, int col)
    -> std::vector<std::pair<Direction, CellPosition>> {
  std::vector<std::pair<Direction, CellPosition>> result;
  const auto current_cell_id =
      topology.IndexFor(MazeDomain::Position2D{.row = row, .col = col});
  if (!current_cell_id.has_value()) {
    return result;
  }

  const std::vector<MazeDomain::CellId> neighbors =
      topology.AdjacentCells(*current_cell_id);
  result.reserve(neighbors.size());
  for (const MazeDomain::CellId neighbor_id : neighbors) {
    const auto neighbor_pos = topology.PositionFor(neighbor_id);
    if (!neighbor_pos.has_value()) {
      continue;
    }
    const CellPosition next_cell = {neighbor_pos->row, neighbor_pos->col};
    const auto dir =
        TryDirectionFromTo(CellPosition{row, col}, next_cell);
    if (!dir.has_value()) {
      continue;
    }
    result.emplace_back(*dir, next_cell);
  }
  return result;
}

void Carve(Grid& grid, int row, int col, Direction dir) {
  const auto kInfo = kDirectionTable[DirectionIndex(dir)];
  int next_row = row + kInfo.dr;
  int next_col = col + kInfo.dc;
  grid[row][col].walls[DirectionIndex(dir)] = false;
  grid[next_row][next_col].walls[DirectionIndex(kInfo.opposite)] = false;
}

auto CreateVisitedGrid(GridSize grid_size)
    -> std::vector<std::vector<bool>> {
  const auto kHeightSize = static_cast<size_t>(grid_size.height);
  const auto kWidthSize = static_cast<size_t>(grid_size.width);
  return std::vector<std::vector<bool>>(kHeightSize,
                                        std::vector<bool>(kWidthSize, false));
}

void ResetWallsForDivision(Grid& maze_grid, int width, int height) {
  for (int row_index = 0; row_index < height; ++row_index) {
    for (int col_index = 0; col_index < width; ++col_index) {
      for (bool& wall : maze_grid[row_index][col_index].walls) {
        wall = false;
      }
      if (row_index == 0) {
        maze_grid[row_index][col_index]
            .walls[DirectionIndex(Direction::Up)] = true;
      }
      if (row_index == height - 1) {
        maze_grid[row_index][col_index]
            .walls[DirectionIndex(Direction::Down)] = true;
      }
      if (col_index == 0) {
        maze_grid[row_index][col_index]
            .walls[DirectionIndex(Direction::Left)] = true;
      }
      if (col_index == width - 1) {
        maze_grid[row_index][col_index]
            .walls[DirectionIndex(Direction::Right)] = true;
      }
    }
  }
}

void AddHorizontalWall(Grid& maze_grid, int wall_row, int col_start,
                       int col_end, int gap_col) {
  for (int col_index = col_start; col_index <= col_end; ++col_index) {
    if (col_index == gap_col) {
      continue;
    }
    maze_grid[wall_row][col_index]
        .walls[DirectionIndex(Direction::Down)] = true;
    maze_grid[wall_row + 1][col_index]
        .walls[DirectionIndex(Direction::Up)] = true;
  }
}

void AddVerticalWall(Grid& maze_grid, int wall_col, int row_start, int row_end,
                     int gap_row) {
  for (int row_index = row_start; row_index <= row_end; ++row_index) {
    if (row_index == gap_row) {
      continue;
    }
    maze_grid[row_index][wall_col]
        .walls[DirectionIndex(Direction::Right)] = true;
    maze_grid[row_index][wall_col + 1]
        .walls[DirectionIndex(Direction::Left)] = true;
  }
}

void DivideRegion(Grid& maze_grid, int row_start, int row_end, int col_start,
                  int col_end, std::mt19937& engine) {
  const int kRegionHeight = row_end - row_start + 1;
  const int kRegionWidth = col_end - col_start + 1;
  if (kRegionHeight < kMinDivisionSpan || kRegionWidth < kMinDivisionSpan) {
    return;
  }

  bool divide_horizontally = false;
  if (kRegionHeight > kRegionWidth) {
    divide_horizontally = true;
  } else if (kRegionWidth > kRegionHeight) {
    divide_horizontally = false;
  } else {
    std::bernoulli_distribution coin_flip(0.5);
    divide_horizontally = coin_flip(engine);
  }

  if (divide_horizontally) {
    std::uniform_int_distribution<int> wall_dist(row_start, row_end - 1);
    std::uniform_int_distribution<int> gap_dist(col_start, col_end);
    const int kWallRow = wall_dist(engine);
    const int kGapCol = gap_dist(engine);
    AddHorizontalWall(maze_grid, kWallRow, col_start, col_end, kGapCol);
    DivideRegion(maze_grid, row_start, kWallRow, col_start, col_end, engine);
    DivideRegion(maze_grid, kWallRow + 1, row_end, col_start, col_end, engine);
  } else {
    std::uniform_int_distribution<int> wall_dist(col_start, col_end - 1);
    std::uniform_int_distribution<int> gap_dist(row_start, row_end);
    const int kWallCol = wall_dist(engine);
    const int kGapRow = gap_dist(engine);
    AddVerticalWall(maze_grid, kWallCol, row_start, row_end, kGapRow);
    DivideRegion(maze_grid, row_start, row_end, col_start, kWallCol, engine);
    DivideRegion(maze_grid, row_start, row_end, kWallCol + 1, col_end, engine);
  }
}

void GenerateMazeRecursiveInternal(int row, int col, Grid& current_maze_data,
                                   std::vector<std::vector<bool>>& visited,
                                   const Topology2D& topology) {
  visited[row][col] = true;
  auto neighbors = AdjacentWithDirection(topology, row, col);

  std::random_device random_device;
  std::mt19937 engine(random_device());
  std::shuffle(neighbors.begin(), neighbors.end(), engine);

  for (const auto& neighbor : neighbors) {
    const Direction dir = neighbor.first;
    const int next_row = neighbor.second.first;
    const int next_col = neighbor.second.second;
    if (!visited[next_row][next_col]) {
      Carve(current_maze_data, row, col, dir);
      GenerateMazeRecursiveInternal(next_row, next_col, current_maze_data,
                                    visited, topology);
    }
  }
}

struct FrontierEdge {
  int row1;
  int col1;
  int row2;
  int col2;
  Direction dir_from_r1;  // direction from (row1,col1) to (row2,col2)
};

void GenerateMazePrimsInternal(int start_row, int start_col,
                               Grid& current_maze_data, int width, int height) {
  const GridSize kGridSize{.width = width, .height = height};
  auto visited = CreateVisitedGrid(kGridSize);
  for (int row_index = 0; row_index < height; ++row_index) {
    for (int col_index = 0; col_index < width; ++col_index) {
      for (bool& wall : current_maze_data[row_index][col_index].walls) {
        wall = true;
      }
    }
  }

  std::random_device random_device;
  std::mt19937 engine(random_device());

  const Topology2D topology(MazeDomain::GridSize2D{.height = height,
                                                    .width = width});
  visited[start_row][start_col] = true;
  std::vector<FrontierEdge> frontier_walls;

  for (const auto& neighbor :
       AdjacentWithDirection(topology, start_row, start_col)) {
    frontier_walls.push_back({start_row, start_col, neighbor.second.first,
                              neighbor.second.second, neighbor.first});
  }

  while (!frontier_walls.empty()) {
    std::uniform_int_distribution<> distrib(
        0, static_cast<int>(frontier_walls.size()) - 1);
    int random_index = distrib(engine);
    FrontierEdge frontier_edge = frontier_walls[random_index];

    frontier_walls[random_index] = frontier_walls.back();
    frontier_walls.pop_back();

    if (!visited[frontier_edge.row2][frontier_edge.col2]) {
      Carve(current_maze_data, frontier_edge.row1, frontier_edge.col1,
            frontier_edge.dir_from_r1);
      visited[frontier_edge.row2][frontier_edge.col2] = true;

      for (const auto& neighbor :
           AdjacentWithDirection(topology, frontier_edge.row2,
                                 frontier_edge.col2)) {
        const int next_row = neighbor.second.first;
        const int next_col = neighbor.second.second;
        if (!visited[next_row][next_col]) {
          frontier_walls.push_back({frontier_edge.row2, frontier_edge.col2,
                                    next_row, next_col, neighbor.first});
        }
      }
    }
  }
}

struct WallEdge {
  int row1;
  int col1;
  int row2;
  int col2;
  Direction dir_from_r1;
};

class DSU {
 public:
  explicit DSU(int n) : parent_(n), rank_(n, 0) {
    std::ranges::iota(parent_, 0);
  }

  auto Find(int index) -> int {
    if (parent_[index] == index) {
      return index;
    }
    return parent_[index] = Find(parent_[index]);
  }

  void Unite(int left_index, int right_index) {
    int root_i = Find(left_index);
    int root_j = Find(right_index);
    if (root_i != root_j) {
      if (rank_[root_i] < rank_[root_j]) {
        std::swap(root_i, root_j);
      }
      parent_[root_j] = root_i;
      if (rank_[root_i] == rank_[root_j]) {
        rank_[root_i]++;
      }
    }
  }

 private:
  std::vector<int> parent_;
  std::vector<int> rank_;
};

void GenerateMazeKruskalInternal(Grid& current_maze_data, int width,
                                 int height) {
  for (int row_index = 0; row_index < height; ++row_index) {
    for (int col_index = 0; col_index < width; ++col_index) {
      for (bool& wall : current_maze_data[row_index][col_index].walls) {
        wall = true;
      }
    }
  }

  const Topology2D topology(MazeDomain::GridSize2D{.height = height,
                                                    .width = width});
  std::vector<WallEdge> all_walls;
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      for (const auto& neighbor : AdjacentWithDirection(topology, row, col)) {
        const Direction dir = neighbor.first;
        if (dir != Direction::Right && dir != Direction::Down) {
          continue;
        }
        all_walls.push_back(
            {row, col, neighbor.second.first, neighbor.second.second, dir});
      }
    }
  }

  std::random_device random_device;
  std::mt19937 engine(random_device());
  std::shuffle(all_walls.begin(), all_walls.end(), engine);

  DSU dsu(width * height);
  int num_edges_added = 0;
  int total_cells = width * height;

  for (const auto& wall_edge : all_walls) {
    if (num_edges_added >= total_cells - 1 && total_cells > 0) {
      break;
    }

    const auto cell1_index = topology.IndexFor(
        MazeDomain::Position2D{.row = wall_edge.row1, .col = wall_edge.col1});
    const auto cell2_index = topology.IndexFor(
        MazeDomain::Position2D{.row = wall_edge.row2, .col = wall_edge.col2});
    if (!cell1_index.has_value() || !cell2_index.has_value()) {
      continue;
    }

    if (dsu.Find(*cell1_index) != dsu.Find(*cell2_index)) {
      Carve(current_maze_data, wall_edge.row1, wall_edge.col1,
            wall_edge.dir_from_r1);
      dsu.Unite(*cell1_index, *cell2_index);
      num_edges_added++;
    }
  }
}

void GenerateMazeDfs(Grid& maze, int start_row, int start_col, int width,
                     int height) {
  const GridSize kGridSize{.width = width, .height = height};
  const Topology2D topology(MazeDomain::GridSize2D{.height = height,
                                                    .width = width});
  auto visited = CreateVisitedGrid(kGridSize);
  GenerateMazeRecursiveInternal(start_row, start_col, maze, visited, topology);
}

void GenerateMazePrims(Grid& maze, int start_row, int start_col, int width,
                       int height) {
  GenerateMazePrimsInternal(start_row, start_col, maze, width, height);
}

void GenerateMazeKruskal(Grid& maze, int /*start_row*/, int /*start_col*/,
                         int width, int height) {
  GenerateMazeKruskalInternal(maze, width, height);
}

void GenerateMazeRecursiveDivision(Grid& maze, int /*start_row*/,
                                   int /*start_col*/, int width, int height) {
  if (width < kMinDivisionSpan || height < kMinDivisionSpan) {
    ResetWallsForDivision(maze, width, height);
    return;
  }
  ResetWallsForDivision(maze, width, height);
  std::random_device random_device;
  std::mt19937 engine(random_device());
  DivideRegion(maze, 0, height - 1, 0, width - 1, engine);
}

void GenerateMazeGrowingTree(Grid& maze, int start_row, int start_col, int width,
                             int height) {
  const GridSize kGridSize{.width = width, .height = height};
  const Topology2D topology(MazeDomain::GridSize2D{.height = height,
                                                    .width = width});
  auto visited = CreateVisitedGrid(kGridSize);
  std::vector<CellPosition> active_cells;
  active_cells.push_back({start_row, start_col});
  visited[start_row][start_col] = true;

  std::random_device random_device;
  std::mt19937 engine(random_device());

  while (!active_cells.empty()) {
    std::uniform_int_distribution<int> index_dist(
        0, static_cast<int>(active_cells.size()) - 1);
    const int kCellIndex = index_dist(engine);
    const CellPosition kCell = active_cells[kCellIndex];

    std::vector<std::pair<Direction, CellPosition>> neighbors;
    for (const auto& neighbor :
         AdjacentWithDirection(topology, kCell.first, kCell.second)) {
      const int kNextRow = neighbor.second.first;
      const int kNextCol = neighbor.second.second;
      if (!visited[kNextRow][kNextCol]) {
        neighbors.push_back(neighbor);
      }
    }

    if (neighbors.empty()) {
      active_cells[kCellIndex] = active_cells.back();
      active_cells.pop_back();
      continue;
    }

    std::uniform_int_distribution<int> dir_dist(
        0, static_cast<int>(neighbors.size()) - 1);
    const auto kNext = neighbors[dir_dist(engine)];
    const Direction kDir = kNext.first;
    const int kNextRow = kNext.second.first;
    const int kNextCol = kNext.second.second;
    Carve(maze, kCell.first, kCell.second, kDir);
    visited[kNextRow][kNextCol] = true;
    active_cells.push_back({kNextRow, kNextCol});
  }
}

}  // namespace

namespace MazeDomain {

MazeGeneratorFactory::MazeGeneratorFactory() {
  register_generator(MazeAlgorithmType::DFS, "DFS", GenerateMazeDfs);
  register_generator(MazeAlgorithmType::PRIMS, "Prims", GenerateMazePrims);
  register_generator(MazeAlgorithmType::KRUSKAL, "Kruskal",
                     GenerateMazeKruskal);
  register_generator(MazeAlgorithmType::RECURSIVE_DIVISION,
                     "Recursive Division", GenerateMazeRecursiveDivision);
  register_generator(MazeAlgorithmType::GROWING_TREE, "Growing Tree",
                     GenerateMazeGrowingTree);
}

auto MazeGeneratorFactory::instance() -> MazeGeneratorFactory& {
  static MazeGeneratorFactory instance;
  return instance;
}

void MazeGeneratorFactory::register_generator(MazeAlgorithmType type,
                                              std::string name,
                                              Generator generator) {
  registry_[type] =
      Entry{.name = std::move(name), .generator = std::move(generator)};

  std::string key;
  key.reserve(registry_[type].name.size());
  for (unsigned char character : registry_[type].name) {
    key.push_back(static_cast<char>(std::toupper(character)));
  }
  name_to_type_[key] = type;
}

auto MazeGeneratorFactory::has_generator(MazeAlgorithmType type) const -> bool {
  return registry_.contains(type);
}

auto MazeGeneratorFactory::get_generator(MazeAlgorithmType type) const
    -> MazeGeneratorFactory::Generator {
  auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.generator;
  }
  return {};
}

auto MazeGeneratorFactory::name_for(MazeAlgorithmType type) const
    -> std::string {
  auto iterator = registry_.find(type);
  if (iterator != registry_.end()) {
    return iterator->second.name;
  }
  return {};
}

auto MazeGeneratorFactory::try_parse(std::string_view name,
                                     MazeAlgorithmType& out_type) const
    -> bool {
  std::string key;
  key.reserve(name.size());
  for (unsigned char character : name) {
    key.push_back(static_cast<char>(std::toupper(character)));
  }
  auto iterator = name_to_type_.find(key);
  if (iterator == name_to_type_.end()) {
    return false;
  }
  out_type = iterator->second;
  return true;
}

auto MazeGeneratorFactory::names() const -> std::vector<std::string> {
  std::vector<std::string> result;
  result.reserve(registry_.size());
  for (const auto& entry : registry_) {
    result.push_back(entry.second.name);
  }
  return result;
}

void generate_maze_structure(MazeGrid& maze_grid_to_populate, int start_r,
                             int start_c, int grid_width, int grid_height,
                             MazeAlgorithmType algorithm_type) {
  if (algorithm_type == MazeAlgorithmType::DFS ||
      algorithm_type == MazeAlgorithmType::PRIMS ||
      algorithm_type == MazeAlgorithmType::GROWING_TREE) {
    if (start_r < 0 || start_r >= grid_height || start_c < 0 ||
        start_c >= grid_width) {
      start_r = 0;
      start_c = 0;
    }
  }

  for (int row_index = 0; row_index < grid_height; ++row_index) {
    for (int col_index = 0; col_index < grid_width; ++col_index) {
      for (bool& wall : maze_grid_to_populate[row_index][col_index].walls) {
        wall = true;
      }
    }
  }

  auto generator =
      MazeGeneratorFactory::instance().get_generator(algorithm_type);
  if (!generator) {
    generator =
        MazeGeneratorFactory::instance().get_generator(MazeAlgorithmType::DFS);
  }
  if (generator) {
    generator(maze_grid_to_populate, start_r, start_c, grid_width, grid_height);
  }
}

auto algorithm_name(MazeAlgorithmType algorithm_type) -> std::string {
  return MazeGeneratorFactory::instance().name_for(algorithm_type);
}

auto try_parse_algorithm(std::string_view name, MazeAlgorithmType& out_type)
    -> bool {
  return MazeGeneratorFactory::instance().try_parse(name, out_type);
}

auto supported_algorithms() -> std::vector<std::string> {
  return MazeGeneratorFactory::instance().names();
}

}  // namespace MazeDomain
