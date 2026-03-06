#ifndef MAZE_EXPORT_MAZE_EXPORT_H
#define MAZE_EXPORT_MAZE_EXPORT_H

#include <string>
#include <string_view>
#include <vector>

#include "config/config.h"
#include "domain/maze_grid.h"
#include "domain/maze_solver.h"

namespace MazeExport {

struct ExportResult {
  bool ok = true;
  std::string error;
  std::string output_folder;
  size_t frames_written = 0;
};

auto ExportFramesToPngSequence(
    const std::vector<MazeSolverDomain::SearchFrame>& frames,
    const MazeDomain::MazeGrid& maze_ref, const Config::AppConfig& config,
    std::string_view sequence_name) -> ExportResult;

}  // namespace MazeExport

#endif  // MAZE_EXPORT_MAZE_EXPORT_H
