#include "infrastructure/graphics/maze_renderer.h"

#include <string_view>

#include "graphics/render_output_png.h"
#include "graphics/render_raster.h"

namespace MazeSolver {

auto RenderFrameToBuffer(const MazeSolverDomain::SearchFrame& frame,
                         const MazeDomain::MazeGrid& maze_ref,
                         const Config::AppConfig& config,
                         RenderBuffer& out_buffer, std::string& error) -> bool {
  return detail::RenderFrameToBufferImpl(frame, maze_ref, config, out_buffer,
                                         error);
}

auto WritePngFile(const std::string& path, const RenderBuffer& buffer,
                  std::string& error) -> bool {
  return detail::WritePngFileImpl(path, buffer, error);
}

auto RenderSearchResult(const MazeSolverDomain::SearchResult& result,
                        const MazeDomain::MazeGrid& maze_ref,
                        MazeSolverDomain::SolverAlgorithmType algorithm_type,
                        std::string_view generation_algorithm_name,
                        const Config::AppConfig& config) -> RenderResult {
  (void)result;
  (void)maze_ref;
  (void)algorithm_type;
  (void)generation_algorithm_name;
  (void)config;
  RenderResult render_result;
  render_result.ok = false;
  render_result.error =
      "RenderSearchResult is removed in event-stream mode. "
      "Render with streamed SearchFrame events instead.";
  return render_result;
}

}  // namespace MazeSolver
