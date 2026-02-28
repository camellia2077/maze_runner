#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "cli/framework/cli_app.h"
#include "cli/commands/generation_algorithms_command.h"
#include "cli/commands/search_algorithms_command.h"
#include "common/version.hpp"
#include "config/config.h"

namespace {

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

auto CreateBaseConfig() -> Config::AppConfig {
  Config::AppConfig config;
  config.maze.width = 5;
  config.maze.height = 5;
  config.maze.generation_algorithms = {
      {MazeGeneration::MazeAlgorithmType::DFS,
       MazeGeneration::algorithm_name(MazeGeneration::MazeAlgorithmType::DFS)}};
  config.maze.search_algorithms = {
      {MazeSolverDomain::SolverAlgorithmType::BFS,
       MazeSolverDomain::AlgorithmName(MazeSolverDomain::SolverAlgorithmType::BFS)}};
  return config;
}

auto RunCli(Cli::CliApp& app, std::vector<std::string> argv_tokens,
            Config::AppConfig& config, std::ostringstream& out_stream,
            std::ostringstream& err_stream, bool& handled) -> int {
  std::vector<char*> argv_ptrs;
  argv_ptrs.reserve(argv_tokens.size());
  for (std::string& token : argv_tokens) {
    argv_ptrs.push_back(token.data());
  }

  Cli::CommandContext context{
      .config = config, .out = out_stream, .err = err_stream};
  return app.run(static_cast<int>(argv_ptrs.size()), argv_ptrs.data(), context,
                 handled);
}

}  // namespace

auto RunCliAppTests() -> int {
  int failures = 0;
  Cli::CliApp app;
  Config::AppConfig config = CreateBaseConfig();
  Cli::RegisterGenerationAlgorithmsCommand(app);
  Cli::RegisterSearchAlgorithmsCommand(app);

  app.register_command({.name = "run",
                        .description = "test run command",
                        .handler = [](const std::vector<std::string>&,
                                      Cli::CommandContext&) -> int { return 0; },
                        .exit_after = false});

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code =
        RunCli(app, {"maze_app", "--output", "out_dir"}, config, out_stream,
               err_stream, handled);
    ExpectEqual(exit_code, 0, "Output option should return success", failures);
    ExpectTrue(handled, "No command after options should be handled", failures);
    ExpectEqual(config.output_dir, std::string("out_dir"),
                "Output directory should be updated", failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code =
        RunCli(app, {"maze_app", "--config", "config/test.toml", "run"}, config,
               out_stream, err_stream, handled);
    ExpectEqual(exit_code, 0, "Config option with run command should succeed",
                failures);
    ExpectTrue(!handled, "Run command with exit_after=false should not exit",
               failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code = RunCli(app, {"maze_app", "--generation-algorithms",
                                       "DFS,KRUSKAL"},
                                 config, out_stream, err_stream, handled);
    ExpectEqual(exit_code, 0, "Generation override should succeed", failures);
    ExpectTrue(
        static_cast<int>(config.maze.generation_algorithms.size()) == 2,
        "Generation algorithms should be updated from CLI option", failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code = RunCli(app, {"maze_app", "--search-algorithms",
                                       "BFS,DFS"},
                                 config, out_stream, err_stream, handled);
    ExpectEqual(exit_code, 0, "Search override should succeed", failures);
    ExpectTrue(static_cast<int>(config.maze.search_algorithms.size()) == 2,
               "Search algorithms should be updated from CLI option",
               failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code =
        RunCli(app, {"maze_app", "--help"}, config, out_stream, err_stream,
               handled);
    ExpectEqual(exit_code, 0, "Help option should succeed", failures);
    ExpectTrue(handled, "Help option should mark handled", failures);
    ExpectTrue(out_stream.str().find("Options:") != std::string::npos,
               "Help output should contain options section", failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code = RunCli(app, {"maze_app", "--version"}, config,
                                 out_stream, err_stream, handled);
    ExpectEqual(exit_code, 0, "Version option should succeed", failures);
    ExpectTrue(handled, "Version option should mark handled", failures);
    ExpectTrue(out_stream.str().find(std::string(MazeCommon::kVersion)) !=
                   std::string::npos,
               "Version output should contain project version", failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code =
        RunCli(app, {"maze_app", "--output"}, config, out_stream, err_stream,
               handled);
    ExpectEqual(exit_code, 1, "Missing output value should fail", failures);
    ExpectTrue(handled, "Missing output value should mark handled", failures);
    ExpectTrue(err_stream.str().find("Missing value") != std::string::npos,
               "Missing output value should print error", failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code =
        RunCli(app, {"maze_app", "--config"}, config, out_stream, err_stream,
               handled);
    ExpectEqual(exit_code, 1, "Missing config value should fail", failures);
    ExpectTrue(handled, "Missing config value should mark handled", failures);
    ExpectTrue(err_stream.str().find("Missing value") != std::string::npos,
               "Missing config value should print error", failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code =
        RunCli(app, {"maze_app", "generation-algorithms"}, config, out_stream,
               err_stream, handled);
    ExpectEqual(exit_code, 0,
                "Generation algorithms list command should succeed", failures);
    ExpectTrue(out_stream.str().find("[2D]") != std::string::npos,
               "Generation algorithms list should include dimension metadata",
               failures);
  }

  {
    std::ostringstream out_stream;
    std::ostringstream err_stream;
    bool handled = false;
    const int exit_code = RunCli(app, {"maze_app", "search-algorithms"},
                                 config, out_stream, err_stream, handled);
    ExpectEqual(exit_code, 0,
                "Search algorithms list command should succeed", failures);
    ExpectTrue(out_stream.str().find("[2D]") != std::string::npos,
               "Search algorithms list should include dimension metadata",
               failures);
  }

  return failures;
}
