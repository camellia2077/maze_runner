#include "domain/maze_generation_algorithms_internal.h"

#include <random>

#include "domain/maze_generation_grid_ops.h"

namespace MazeDomain::MazeGenerationDetail {
namespace {

constexpr int kMinDivisionSpan = 2;

void ResetWallsForDivision(Grid& maze_grid, int width, int height) {
  for (int row_index = 0; row_index < height; ++row_index) {
    for (int col_index = 0; col_index < width; ++col_index) {
      for (bool& wall : maze_grid[row_index][col_index].walls) {
        wall = false;
      }
      if (row_index == 0) {
        maze_grid[row_index][col_index].walls[DirectionIndex(Direction::Up)] = true;
      }
      if (row_index == height - 1) {
        maze_grid[row_index][col_index].walls[DirectionIndex(Direction::Down)] =
            true;
      }
      if (col_index == 0) {
        maze_grid[row_index][col_index].walls[DirectionIndex(Direction::Left)] =
            true;
      }
      if (col_index == width - 1) {
        maze_grid[row_index][col_index].walls[DirectionIndex(Direction::Right)] =
            true;
      }
    }
  }
}

void AddHorizontalWall(Grid& maze_grid, int wall_row, int col_start, int col_end,
                       int gap_col) {
  for (int col_index = col_start; col_index <= col_end; ++col_index) {
    if (col_index == gap_col) {
      continue;
    }
    maze_grid[wall_row][col_index].walls[DirectionIndex(Direction::Down)] = true;
    maze_grid[wall_row + 1][col_index].walls[DirectionIndex(Direction::Up)] = true;
  }
}

void AddVerticalWall(Grid& maze_grid, int wall_col, int row_start, int row_end,
                     int gap_row) {
  for (int row_index = row_start; row_index <= row_end; ++row_index) {
    if (row_index == gap_row) {
      continue;
    }
    maze_grid[row_index][wall_col].walls[DirectionIndex(Direction::Right)] = true;
    maze_grid[row_index][wall_col + 1].walls[DirectionIndex(Direction::Left)] =
        true;
  }
}

void DivideRegion(Grid& maze_grid, int row_start, int row_end, int col_start,
                  int col_end, std::mt19937& engine) {
  const int region_height = row_end - row_start + 1;
  const int region_width = col_end - col_start + 1;
  if (region_height < kMinDivisionSpan || region_width < kMinDivisionSpan) {
    return;
  }

  bool divide_horizontally = false;
  if (region_height > region_width) {
    divide_horizontally = true;
  } else if (region_width > region_height) {
    divide_horizontally = false;
  } else {
    std::bernoulli_distribution coin_flip(0.5);
    divide_horizontally = coin_flip(engine);
  }

  if (divide_horizontally) {
    std::uniform_int_distribution<int> wall_dist(row_start, row_end - 1);
    std::uniform_int_distribution<int> gap_dist(col_start, col_end);
    const int wall_row = wall_dist(engine);
    const int gap_col = gap_dist(engine);
    AddHorizontalWall(maze_grid, wall_row, col_start, col_end, gap_col);
    DivideRegion(maze_grid, row_start, wall_row, col_start, col_end, engine);
    DivideRegion(maze_grid, wall_row + 1, row_end, col_start, col_end, engine);
  } else {
    std::uniform_int_distribution<int> wall_dist(col_start, col_end - 1);
    std::uniform_int_distribution<int> gap_dist(row_start, row_end);
    const int wall_col = wall_dist(engine);
    const int gap_row = gap_dist(engine);
    AddVerticalWall(maze_grid, wall_col, row_start, row_end, gap_row);
    DivideRegion(maze_grid, row_start, row_end, col_start, wall_col, engine);
    DivideRegion(maze_grid, row_start, row_end, wall_col + 1, col_end, engine);
  }
}

}  // namespace

void GenerateMazeRecursiveDivision(MazeGrid& maze, int /*start_row*/,
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

}  // namespace MazeDomain::MazeGenerationDetail
