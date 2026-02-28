#include <iostream>
#include <vector>

#include "domain/maze_solver.h"

namespace {

constexpr int kWallTop = 0;
constexpr int kWallRight = 1;
constexpr int kWallBottom = 2;
constexpr int kWallLeft = 3;

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

auto CreateClosedGrid(int height, int width) -> MazeDomain::MazeGrid {
  return {static_cast<size_t>(height),
          MazeDomain::MazeGrid::value_type(static_cast<size_t>(width))};
}

void OpenPassage(MazeDomain::MazeGrid& grid, int row_a, int col_a, int row_b,
                 int col_b) {
  if (row_a == row_b && col_b == col_a + 1) {
    grid[row_a][col_a].walls[kWallRight] = false;
    grid[row_b][col_b].walls[kWallLeft] = false;
    return;
  }
  if (row_a == row_b && col_b == col_a - 1) {
    grid[row_a][col_a].walls[kWallLeft] = false;
    grid[row_b][col_b].walls[kWallRight] = false;
    return;
  }
  if (col_a == col_b && row_b == row_a + 1) {
    grid[row_a][col_a].walls[kWallBottom] = false;
    grid[row_b][col_b].walls[kWallTop] = false;
    return;
  }
  if (col_a == col_b && row_b == row_a - 1) {
    grid[row_a][col_a].walls[kWallTop] = false;
    grid[row_b][col_b].walls[kWallBottom] = false;
  }
}

auto SolveWithAllAlgorithms(const MazeDomain::MazeGrid& grid,
                            MazeSolverDomain::GridPosition start,
                            MazeSolverDomain::GridPosition end)
    -> std::vector<MazeSolverDomain::SearchResult> {
  const std::vector<MazeSolverDomain::SolverAlgorithmType> algorithms = {
      MazeSolverDomain::SolverAlgorithmType::BFS,
      MazeSolverDomain::SolverAlgorithmType::DFS,
      MazeSolverDomain::SolverAlgorithmType::ASTAR,
      MazeSolverDomain::SolverAlgorithmType::DIJKSTRA,
      MazeSolverDomain::SolverAlgorithmType::GREEDY_BEST_FIRST};

  std::vector<MazeSolverDomain::SearchResult> results;
  results.reserve(algorithms.size());
  for (const auto algorithm : algorithms) {
    results.push_back(MazeSolverDomain::Solve(grid, start, end, algorithm));
  }
  return results;
}

}  // namespace

auto RunSolverAlgorithmTests() -> int {
  int failures = 0;

  {
    MazeDomain::MazeGrid grid = CreateClosedGrid(2, 2);
    OpenPassage(grid, 0, 0, 0, 1);
    OpenPassage(grid, 0, 1, 1, 1);

    const auto results = SolveWithAllAlgorithms(grid, {0, 0}, {1, 1});
    for (const auto& result : results) {
      ExpectTrue(result.found_, "Path should be found", failures);
      ExpectEqual(result.path_.front(), MazeSolverDomain::GridPosition({0, 0}),
                  "Path should start at start node", failures);
      ExpectEqual(result.path_.back(), MazeSolverDomain::GridPosition({1, 1}),
                  "Path should end at end node", failures);
      ExpectTrue(result.path_.size() >= 3,
                 "Path should contain at least three nodes", failures);
    }
  }

  {
    const MazeDomain::MazeGrid grid = CreateClosedGrid(2, 2);
    const auto results = SolveWithAllAlgorithms(grid, {0, 0}, {1, 1});
    for (const auto& result : results) {
      ExpectTrue(!result.found_, "No path should be found in closed grid",
                 failures);
    }
  }

  {
    const MazeDomain::MazeGrid grid = CreateClosedGrid(2, 2);
    const auto results = SolveWithAllAlgorithms(grid, {1, 1}, {1, 1});
    for (const auto& result : results) {
      ExpectTrue(result.found_, "Trivial path should be found", failures);
      ExpectEqual(result.path_.size(), static_cast<size_t>(1),
                  "Trivial path length should be one", failures);
      ExpectEqual(result.path_.front(), MazeSolverDomain::GridPosition({1, 1}),
                  "Trivial path node should match start/end", failures);
    }
  }

  return failures;
}
