#include "cli/commands/version_command.h"

#include "common/version.hpp"
#include "common/kernel_version.hpp"

namespace Cli {

void RegisterVersionCommand(CliApp& app) {
  Command command;
  command.name = "version";
  command.description = "Print application version";
  command.handler = [](const std::vector<std::string>&,
                       CommandContext& ctx) -> int {
    ctx.out << MazeCommon::kVersionDisplayPrefix << MazeCommon::kCliVersion
            << MazeCommon::kKernelDisplayPrefix << MazeKernel::kKernelVersion
            << "\n";
    return 0;
  };
  app.register_command(std::move(command));
}

}  // namespace Cli
