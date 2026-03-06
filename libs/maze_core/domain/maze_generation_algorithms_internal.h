#ifndef MAZE_DOMAIN_MAZE_GENERATION_ALGORITHMS_INTERNAL_H
#define MAZE_DOMAIN_MAZE_GENERATION_ALGORITHMS_INTERNAL_H

#include "domain/maze_generation.h"

namespace MazeDomain::MazeGenerationDetail {

void GenerateMazeDfs(MazeGrid& maze, int start_row, int start_col, int width,
                     int height);
void GenerateMazePrims(MazeGrid& maze, int start_row, int start_col, int width,
                       int height);
void GenerateMazeKruskal(MazeGrid& maze, int start_row, int start_col, int width,
                         int height);
void GenerateMazeRecursiveDivision(MazeGrid& maze, int start_row, int start_col,
                                   int width, int height);
void GenerateMazeGrowingTree(MazeGrid& maze, int start_row, int start_col,
                             int width, int height);

}  // namespace MazeDomain::MazeGenerationDetail

#endif  // MAZE_DOMAIN_MAZE_GENERATION_ALGORITHMS_INTERNAL_H
