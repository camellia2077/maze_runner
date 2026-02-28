#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "infrastructure/config/config_loader.h"

namespace {

namespace fs = std::filesystem;

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

auto CreateTempTestDir() -> fs::path {
  const fs::path test_dir =
      fs::temp_directory_path() / "maze_generator_config_loader_tests";
  std::error_code fs_error;
  fs::create_directories(test_dir, fs_error);
  return test_dir;
}

auto WriteTextFile(const fs::path& file_path, std::string_view content)
    -> bool {
  std::ofstream output_file(file_path, std::ios::binary);
  if (!output_file.is_open()) {
    return false;
  }
  output_file << content;
  return output_file.good();
}

}  // namespace

auto RunConfigLoaderTests() -> int {
  int failures = 0;
  const fs::path test_dir = CreateTempTestDir();

  const fs::path valid_config_path = test_dir / "valid_config.toml";
  const std::string valid_config = R"([MazeConfig]
MazeWidth = 7
MazeHeight = 6
UnitPixels = 12
StartNodeX = 1
StartNodeY = 2
EndNodeX = 6
EndNodeY = 5
GenerationAlgorithms = ["DFS","KRUSKAL"]
SearchAlgorithms = ["BFS","DFS"]

[ColorConfig]
BackgroundColor = "#FFFFFF"
OuterWallColor = "#4A4A4A"
InnerWallColor = "#111111"
StartNodeColor = "#00FF00"
EndNodeColor = "#FF0000"
)";
  ExpectTrue(WriteTextFile(valid_config_path, valid_config),
             "Should write valid config file", failures);
  const ConfigLoader::LoadResult valid_result =
      ConfigLoader::load_config(valid_config_path.string());
  ExpectTrue(valid_result.ok, "Valid config should load successfully", failures);
  ExpectEqual(valid_result.config.maze.width, 7,
              "Maze width should match config", failures);
  ExpectEqual(valid_result.config.maze.height, 6,
              "Maze height should match config", failures);
  ExpectEqual(valid_result.config.maze.unit_pixels, 12,
              "Unit pixels should match config", failures);
  ExpectEqual(valid_result.config.maze.start_node.first, 2,
              "Start row should match config", failures);
  ExpectEqual(valid_result.config.maze.start_node.second, 1,
              "Start col should match config", failures);
  ExpectEqual(static_cast<int>(valid_result.config.maze.search_algorithms.size()),
              2, "Search algorithm count should match config", failures);

  const fs::path invalid_search_config_path = test_dir / "invalid_search.toml";
  const std::string invalid_search_config = R"([MazeConfig]
MazeWidth = 5
MazeHeight = 5
GenerationAlgorithms = ["DFS"]
SearchAlgorithms = ["NotARealAlgo"]
)";
  ExpectTrue(WriteTextFile(invalid_search_config_path, invalid_search_config),
             "Should write invalid-search config file", failures);
  const ConfigLoader::LoadResult invalid_search_result =
      ConfigLoader::load_config(invalid_search_config_path.string());
  ExpectTrue(!invalid_search_result.ok,
             "Unknown search algorithms should fail loading", failures);
  ExpectTrue(invalid_search_result.error.find("SearchAlgorithms") !=
                 std::string::npos,
             "Invalid search should report SearchAlgorithms error", failures);
  ExpectTrue(!invalid_search_result.warnings.empty(),
             "Invalid search should produce warnings", failures);

  const fs::path valid_3d_config_path = test_dir / "valid_3d_config.toml";
  const std::string valid_3d_config = R"([MazeConfig]
Dimension = 3
MazeWidth = 4
MazeHeight = 3
MazeDepth = 2
StartNodeX = 1
StartNodeY = 0
StartNodeZ = 1
EndNodeX = 3
EndNodeY = 2
EndNodeZ = 0
GenerationAlgorithms = ["DFS"]
SearchAlgorithms = ["BFS"]
)";
  ExpectTrue(WriteTextFile(valid_3d_config_path, valid_3d_config),
             "Should write valid 3D config file", failures);
  const ConfigLoader::LoadResult valid_3d_result =
      ConfigLoader::load_config(valid_3d_config_path.string());
  ExpectTrue(valid_3d_result.ok, "Valid 3D config should parse", failures);
  ExpectEqual(valid_3d_result.config.maze.dimension,
              Config::MazeRuntimeDimension::D3,
              "Dimension should be parsed as 3D", failures);
  ExpectEqual(valid_3d_result.config.maze.depth, 2,
              "Maze depth should be parsed for 3D", failures);
  ExpectEqual(valid_3d_result.config.maze.start_node_z, 1,
              "Start Z should be parsed for 3D", failures);
  ExpectEqual(valid_3d_result.config.maze.end_node_z, 0,
              "End Z should be parsed for 3D", failures);

  const fs::path invalid_3d_depth_config_path =
      test_dir / "invalid_3d_depth_config.toml";
  const std::string invalid_3d_depth_config = R"([MazeConfig]
Dimension = 3
MazeWidth = 4
MazeHeight = 3
MazeDepth = 0
GenerationAlgorithms = ["DFS"]
SearchAlgorithms = ["BFS"]
)";
  ExpectTrue(WriteTextFile(invalid_3d_depth_config_path,
                           invalid_3d_depth_config),
             "Should write invalid 3D depth config file", failures);
  const ConfigLoader::LoadResult invalid_3d_depth_result =
      ConfigLoader::load_config(invalid_3d_depth_config_path.string());
  ExpectTrue(!invalid_3d_depth_result.ok,
             "3D config with non-positive depth should fail", failures);
  ExpectTrue(invalid_3d_depth_result.error.find("MazeDepth") !=
                 std::string::npos,
             "Invalid 3D depth should mention MazeDepth", failures);

  return failures;
}
