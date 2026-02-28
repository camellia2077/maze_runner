#ifndef MAZE_USECASE_MAZE_PIPELINE_H
#define MAZE_USECASE_MAZE_PIPELINE_H

#include <ostream>

#include "config/config.h"

namespace MazeUsecase {

enum class PipelineStatus {
  kOk = 0,
  kUnsupportedDimension = 1,
};

auto RunGenerationPipeline(const Config::AppConfig& config, std::ostream& out,
                           std::ostream& err) -> PipelineStatus;

}  // namespace MazeUsecase

#endif  // MAZE_USECASE_MAZE_PIPELINE_H
