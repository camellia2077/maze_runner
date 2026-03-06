#include "export/maze_export.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>

#include "infrastructure/graphics/maze_renderer.h"

namespace MazeExport {
namespace {

namespace fs = std::filesystem;

auto BuildFramePath(const fs::path& folder_path, size_t frame_index) -> fs::path {
  std::ostringstream oss;
  oss << "frame_" << std::setw(4) << std::setfill('0') << frame_index << ".png";
  return folder_path / oss.str();
}

}  // namespace

auto ExportFramesToPngSequence(
    const std::vector<MazeSolverDomain::SearchFrame>& frames,
    const MazeDomain::MazeGrid& maze_ref, const Config::AppConfig& config,
    std::string_view sequence_name) -> ExportResult {
  ExportResult result;
  if (frames.empty()) {
    result.ok = false;
    result.error = "No frames provided for export.";
    return result;
  }
  if (config.output_dir.empty()) {
    result.ok = false;
    result.error = "output_dir must not be empty for export.";
    return result;
  }

  const fs::path folder_path =
      fs::path(config.output_dir) / std::string(sequence_name);
  std::error_code fs_error;
  fs::create_directories(folder_path, fs_error);
  if (fs_error) {
    result.ok = false;
    result.error = "Failed to create export directory: " + fs_error.message();
    return result;
  }

  for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index) {
    MazeSolver::RenderBuffer buffer;
    std::string error;
    if (!MazeSolver::RenderFrameToBuffer(frames[frame_index], maze_ref, config,
                                         buffer, error)) {
      result.ok = false;
      result.error = "RenderFrameToBuffer failed: " + error;
      return result;
    }

    if (!MazeSolver::WritePngFile(BuildFramePath(folder_path, frame_index).string(),
                                  buffer, error)) {
      result.ok = false;
      result.error = "WritePngFile failed: " + error;
      return result;
    }
    result.frames_written += 1;
  }

  result.output_folder = folder_path.string();
  return result;
}

}  // namespace MazeExport
