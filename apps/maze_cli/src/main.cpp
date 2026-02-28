#include <chrono>  // Required for high-precision timing
#include <filesystem>
#include <iomanip>  // Required for std::fixed and std::setprecision
#include <iostream>
#include <string>
#include <vector>

#include "cli/commands/generation_algorithms_command.h"
#include "cli/commands/search_algorithms_command.h"
#include "cli/commands/version_command.h"
#include "cli/framework/cli_app.h"
#include "infrastructure/config/config_loader.h"
#include "usecase/maze_pipeline.h"

// ANSI escape codes for colors
const std::string kResetColor = "\033[0m";
const std::string kGreenColor = "\033[32m";
const std::string kConfigFilename = "config.toml";
const std::string kConfigDirname = "config";
const std::string kConfigOptShort = "-c";
const std::string kConfigOptLong = "--config";

namespace {

using Clock = std::chrono::high_resolution_clock;

auto BuildConfigPath(const char* argv0) -> std::filesystem::path {
  const std::filesystem::path kExePath = std::filesystem::absolute(argv0);
  return kExePath.parent_path() / kConfigDirname / kConfigFilename;
}

auto DimensionLabel(Config::MazeRuntimeDimension dimension) -> const char* {
  if (dimension == Config::MazeRuntimeDimension::D3) {
    return "3D";
  }
  return "2D";
}

struct StartupOptions {
  bool ok = true;
  std::filesystem::path config_path;
  std::string error;
};

auto ParseStartupOptions(int argc, char** argv,
                         std::filesystem::path default_config_path)
    -> StartupOptions {
  StartupOptions options;
  options.config_path = std::move(default_config_path);

  for (int arg_index = 1; arg_index < argc; ++arg_index) {
    const std::string token = argv[arg_index];
    if (token == kConfigOptShort || token == kConfigOptLong) {
      if (arg_index + 1 >= argc) {
        options.ok = false;
        options.error = "Missing value for " + token;
        return options;
      }
      options.config_path = argv[++arg_index];
    }
  }

  return options;
}

void PrintAlgorithmList(
    const std::vector<Config::AlgorithmInfo>& algorithms) {
  for (size_t index = 0; index < algorithms.size(); ++index) {
    std::cout << algorithms[index].name;
    if (index + 1 < algorithms.size()) {
      std::cout << ", ";
    }
  }
  std::cout << std::endl;
}

void PrintSearchAlgorithmList(
    const std::vector<Config::SearchAlgorithmInfo>& algorithms) {
  for (size_t index = 0; index < algorithms.size(); ++index) {
    std::cout << algorithms[index].name;
    if (index + 1 < algorithms.size()) {
      std::cout << ", ";
    }
  }
  std::cout << std::endl;
}

void PrintConfigSummary(const Config::AppConfig& config,
                        const std::filesystem::path& config_path) {
  std::cout << "Configuration successfully loaded from "
            << config_path.string() << std::endl;
  std::cout << "Maze Runtime Dimension: "
            << DimensionLabel(config.maze.dimension) << std::endl;
  std::cout << "Maze Dimensions: " << config.maze.width << "x"
            << config.maze.height;
  if (config.maze.dimension == Config::MazeRuntimeDimension::D3) {
    std::cout << "x" << config.maze.depth;
  }
  std::cout << ", Unit Pixels: " << config.maze.unit_pixels << std::endl;
  std::cout << "Start Node: (" << config.maze.start_node.first << ","
            << config.maze.start_node.second;
  if (config.maze.dimension == Config::MazeRuntimeDimension::D3) {
    std::cout << "," << config.maze.start_node_z;
  }
  std::cout << "), End Node: (" << config.maze.end_node.first << ","
            << config.maze.end_node.second;
  if (config.maze.dimension == Config::MazeRuntimeDimension::D3) {
    std::cout << "," << config.maze.end_node_z;
  }
  std::cout << ")" << std::endl;
  std::cout << "Selected Generation Algorithms: ";
  PrintAlgorithmList(config.maze.generation_algorithms);
  std::cout << "Selected Search Algorithms: ";
  PrintSearchAlgorithmList(config.maze.search_algorithms);
}

void PrintLoadWarnings(const std::vector<std::string>& warnings) {
  for (const auto& warning : warnings) {
    std::cerr << warning << "\n";
  }
}

void RegisterBuiltInCommands(Cli::CliApp& cli) {
  cli.register_command({.name = "run",
                        .description = "Run maze generation + solving pipeline",
                        .handler = [](const std::vector<std::string>& args,
                                      Cli::CommandContext& ctx) -> int {
                          if (!args.empty()) {
                            ctx.err << "Unknown option: " << args.front()
                                    << "\n";
                            return 1;
                          }
                          return 0;
                        },
                        .exit_after = false});
  cli.register_command({.name = "help",
                        .description = "Show available commands",
                        .handler = [&cli](const std::vector<std::string>&,
                                          Cli::CommandContext& ctx) -> int {
                          cli.print_help(ctx.out);
                          return 0;
                        }});
}

auto RunCli(Cli::CliApp& cli,
            int argc,
            char** argv,
            Config::AppConfig& config,
            int& exit_code) -> bool {
  Cli::CommandContext cli_ctx{
      .config = config, .out = std::cout, .err = std::cerr};
  bool handled = false;
  exit_code = cli.run(argc, argv, cli_ctx, handled);
  return handled;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  std::cout << "--- Parameter Loading ---" << std::endl;
  const auto kStartTimeConfig = Clock::now();

  const auto kDefaultConfigPath = BuildConfigPath(argv[0]);
  const StartupOptions startup_options =
      ParseStartupOptions(argc, argv, kDefaultConfigPath);
  if (!startup_options.ok) {
    std::cerr << startup_options.error << "\n";
    return 1;
  }

  const auto kConfigPath = startup_options.config_path;
  ConfigLoader::LoadResult load_result =
      ConfigLoader::load_config(kConfigPath.string());
  Config::AppConfig config = std::move(load_result.config);
  if (!load_result.ok) {
    std::cerr << load_result.error << "\n";
  } else {
    PrintConfigSummary(config, kConfigPath);
  }
  PrintLoadWarnings(load_result.warnings);

  if (config.maze.search_algorithms.empty()) {
    return 1;
  }

  if (!load_result.ok) {
    std::cerr << "Using default values.\n";
  }

  Cli::CliApp cli;
  Cli::RegisterVersionCommand(cli);
  Cli::RegisterGenerationAlgorithmsCommand(cli);
  Cli::RegisterSearchAlgorithmsCommand(cli);
  RegisterBuiltInCommands(cli);

  int cli_code = 0;
  if (RunCli(cli, argc, argv, config, cli_code)) {
    return cli_code;
  }

  const auto kEndTimeConfig = Clock::now();
  const auto kTimeTakenConfig =
      std::chrono::duration<double>(kEndTimeConfig - kStartTimeConfig);
  std::cout << kGreenColor << std::fixed << std::setprecision(3)
            << "Time to load config: " << kTimeTakenConfig.count() << " s"
            << kResetColor << std::endl;

  const auto pipeline_status =
      MazeUsecase::RunGenerationPipeline(config, std::cout, std::cerr);
  if (pipeline_status == MazeUsecase::PipelineStatus::kUnsupportedDimension) {
    return 1;
  }

  std::cout << "\n--- Processing Complete ---" << std::endl;
  std::cout << "All selected algorithms processed. Images saved in respective "
               "folders."
            << std::endl;

  return 0;
}
