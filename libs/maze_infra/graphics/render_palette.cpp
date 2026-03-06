#include "graphics/render_palette.h"

namespace MazeSolver::detail {
namespace {

inline constexpr int kWallRight = 1;
inline constexpr int kWallBottom = 2;

}  // namespace

auto SelectCellColor(MazeCoord cell, const SearchFrame& frame,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors)
    -> const unsigned char* {
  const SolverCellState state = frame.visual_states_[cell.row][cell.col];
  const unsigned char* current_color_ptr = colors.background;
  if (state == SolverCellState::START) {
    current_color_ptr = colors.start;
  } else if (state == SolverCellState::END) {
    current_color_ptr = colors.end;
  } else if (state == SolverCellState::SOLUTION) {
    current_color_ptr = colors.solution_path;
  } else if (state == SolverCellState::CURRENT_PROC) {
    current_color_ptr = colors.current;
  } else if (state == SolverCellState::FRONTIER) {
    current_color_ptr = colors.frontier;
  } else if (state == SolverCellState::VISITED_PROC) {
    current_color_ptr = colors.visited;
  }

  if (state != SolverCellState::SOLUTION) {
    if (cell.row == maze.start_node.first && cell.col == maze.start_node.second &&
        state != SolverCellState::END) {
      current_color_ptr = colors.start;
    }
    if (cell.row == maze.end_node.first && cell.col == maze.end_node.second &&
        state != SolverCellState::START) {
      current_color_ptr = colors.end;
    }
  }

  return current_color_ptr;
}

auto SelectVerticalWallColor(UnitCoord unit,
                             const MazeDomain::MazeGrid& maze_ref,
                             const Config::MazeConfig& maze,
                             const Config::ColorConfig& colors)
    -> const unsigned char* {
  const int maze_row = (unit.row - 1) / kGridSpacing;
  const int maze_col_left = (unit.col / kGridSpacing) - 1;
  if (maze_row >= 0 && maze_row < maze.height && maze_col_left >= 0 &&
      maze_col_left < maze.width) {
    if (!maze_ref[maze_row][maze_col_left].walls[kWallRight]) {
      return colors.background;
    }
    return colors.inner_wall;
  }
  return colors.inner_wall;
}

auto SelectHorizontalWallColor(UnitCoord unit,
                               const MazeDomain::MazeGrid& maze_ref,
                               const Config::MazeConfig& maze,
                               const Config::ColorConfig& colors)
    -> const unsigned char* {
  const int maze_row_up = (unit.row / kGridSpacing) - 1;
  const int maze_col = (unit.col - 1) / kGridSpacing;
  if (maze_row_up >= 0 && maze_row_up < maze.height && maze_col >= 0 &&
      maze_col < maze.width) {
    if (!maze_ref[maze_row_up][maze_col].walls[kWallBottom]) {
      return colors.background;
    }
    return colors.inner_wall;
  }
  return colors.inner_wall;
}

auto SelectUnitColor(UnitCoord unit, const ImageSize& image_size,
                     const SearchFrame& frame,
                     const MazeDomain::MazeGrid& maze_ref,
                     const Config::MazeConfig& maze,
                     const Config::ColorConfig& colors)
    -> const unsigned char* {
  if (IsOuterFrameUnit(unit, image_size)) {
    return colors.outer_wall;
  }
  if (IsCellUnit(unit)) {
    const MazeCoord cell{.row = (unit.row - 1) / kGridSpacing,
                         .col = (unit.col - 1) / kGridSpacing};
    return SelectCellColor(cell, frame, maze, colors);
  }
  if (IsVerticalWallUnit(unit)) {
    return SelectVerticalWallColor(unit, maze_ref, maze, colors);
  }
  if (IsHorizontalWallUnit(unit)) {
    return SelectHorizontalWallColor(unit, maze_ref, maze, colors);
  }
  return colors.inner_wall;
}

}  // namespace MazeSolver::detail
