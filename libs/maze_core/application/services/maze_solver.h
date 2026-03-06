#ifndef MAZE_SOLVER_H
#define MAZE_SOLVER_H

#include <string>
#include <string_view>

#include "config/config.h"
#include "domain/maze_grid.h"
#include "domain/maze_solver.h"

namespace MazeApplication {
struct MazeRuntimeContext;
}

namespace MazeSolver {

using SolverAlgorithmType = MazeSolverDomain::SolverAlgorithmType;
using SearchResult = MazeSolverDomain::SearchResult;
using SolveOptions = MazeSolverDomain::SolveOptions;
using ISearchEventSink = MazeSolverDomain::ISearchEventSink;

std::string AlgorithmName(SolverAlgorithmType algorithm_type);
bool TryParseAlgorithm(std::string_view name, SolverAlgorithmType& out_type);
std::vector<std::string> SupportedAlgorithms();

SearchResult Solve(const MazeDomain::MazeGrid& maze_data,
                   SolverAlgorithmType algorithm_type,
                   const Config::AppConfig& config,
                   ISearchEventSink* event_sink = nullptr,
                   const SolveOptions& options = {});
SearchResult Solve(const MazeApplication::MazeRuntimeContext& runtime_context,
                   ISearchEventSink* event_sink = nullptr,
                   const SolveOptions& options = {});

}  // namespace MazeSolver

#endif  // MAZE_SOLVER_H
