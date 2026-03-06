#include "graphics/render_output_png.h"

#include <filesystem>
#include <string>
#include <system_error>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

namespace MazeSolver::detail {

auto WritePngFileImpl(const std::string& path,
                      const MazeSolver::RenderBuffer& buffer,
                      std::string& error) -> bool {
  if (buffer.width <= 0 || buffer.height <= 0 || buffer.channels <= 0) {
    error = "Invalid render buffer dimensions.";
    return false;
  }
  if (buffer.pixels.empty()) {
    error = "Render buffer is empty.";
    return false;
  }

  const fs::path output_path(path);
  if (output_path.empty()) {
    error = "Output path is empty.";
    return false;
  }

  const fs::path parent = output_path.parent_path();
  if (!parent.empty()) {
    std::error_code create_error;
    fs::create_directories(parent, create_error);
    if (create_error) {
      error = "Failed to create output directory: " + parent.string();
      return false;
    }
  }

  const int write_ok =
      stbi_write_png(path.c_str(), buffer.width, buffer.height, buffer.channels,
                     buffer.pixels.data(), buffer.width * buffer.channels);
  if (write_ok == 0) {
    error = "Failed to write image: " + path;
    return false;
  }
  return true;
}

}  // namespace MazeSolver::detail
