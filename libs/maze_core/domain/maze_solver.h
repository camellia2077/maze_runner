#ifndef MAZE_DOMAIN_MAZE_SOLVER_H
#define MAZE_DOMAIN_MAZE_SOLVER_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "domain/maze_grid.h"

namespace MazeSolverDomain {

using GridPosition = std::pair<int, int>;

enum class SolverAlgorithmType { BFS, DFS, ASTAR, DIJKSTRA, GREEDY_BEST_FIRST };

enum class SolverCellState {
  NONE,
  START,
  END,
  FRONTIER,
  CURRENT_PROC,
  VISITED_PROC,
  SOLUTION
};

struct SearchFrame {
  std::vector<std::vector<SolverCellState>> visual_states_;
  std::vector<GridPosition> current_path_;
};

enum class SearchEventType {
  kRunStarted,
  kCellStateChanged,
  kPathUpdated,
  kProgress,
  kRunFinished,
  kRunCancelled,
  kRunFailed
};

struct CellDelta {
  int row = 0;
  int col = 0;
  SolverCellState state = SolverCellState::NONE;
};

struct SearchEvent {
  uint64_t seq = 0;
  SearchEventType type = SearchEventType::kProgress;
  std::vector<CellDelta> deltas;
  std::vector<GridPosition> path;
  int visited_count = 0;
  int frontier_count = 0;
  bool found = false;
  std::string message;
};

class ISearchEventSink {
 public:
  virtual ~ISearchEventSink() = default;
  virtual void OnEvent(const SearchEvent& event) = 0;
  virtual bool ShouldCancel() const = 0;
};

struct SolveOptions {
  int emit_stride = 1;
  bool emit_progress = true;
};

struct SearchResult {
  bool found_ = false;
  bool cancelled_ = false;
  std::vector<GridPosition> path_;
  std::vector<std::vector<bool>> explored_;
};

class MazeSolverFactory {
 public:
  using Solver = std::function<SearchResult(const MazeDomain::MazeGrid&,
                                            GridPosition start_node,
                                            GridPosition end_node,
                                            ISearchEventSink* event_sink,
                                            const SolveOptions& options)>;

  static MazeSolverFactory& Instance();

  void RegisterSolver(SolverAlgorithmType type, std::string name,
                      Solver solver);
  bool HasSolver(SolverAlgorithmType type) const;
  Solver GetSolver(SolverAlgorithmType type) const;
  std::string NameFor(SolverAlgorithmType type) const;
  bool TryParse(std::string_view name, SolverAlgorithmType& out_type) const;
  std::vector<std::string> Names() const;

 private:
  MazeSolverFactory();

  struct Entry {
    std::string name;
    Solver solver;
  };

  std::map<SolverAlgorithmType, Entry> registry_;
  std::map<std::string, SolverAlgorithmType> name_to_type_;
};

SearchResult Solve(const MazeDomain::MazeGrid& maze_grid,
                   GridPosition start_node, GridPosition end_node,
                   SolverAlgorithmType algorithm_type,
                   ISearchEventSink* event_sink = nullptr,
                   const SolveOptions& options = {});
std::string AlgorithmName(SolverAlgorithmType algorithm_type);
bool TryParseAlgorithm(std::string_view name, SolverAlgorithmType& out_type);
std::vector<std::string> supported_algorithms();

}  // namespace MazeSolverDomain

#endif  // MAZE_DOMAIN_MAZE_SOLVER_H
