#ifndef MAZE_DOMAIN_MAZE_SOLVER_COMMON_H
#define MAZE_DOMAIN_MAZE_SOLVER_COMMON_H

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "domain/grid_topology.h"
#include "domain/maze_solver.h"

namespace MazeSolverDomain::detail {

using MazeGrid = MazeDomain::MazeGrid;
using GridPosition = MazeSolverDomain::GridPosition;
using SearchResult = MazeSolverDomain::SearchResult;
using SearchEvent = MazeSolverDomain::SearchEvent;
using SearchEventType = MazeSolverDomain::SearchEventType;
using SolveOptions = MazeSolverDomain::SolveOptions;
using ISearchEventSink = MazeSolverDomain::ISearchEventSink;
using SolverCellState = MazeSolverDomain::SolverCellState;
using BoolGrid = std::vector<std::vector<bool>>;
using IntGrid = std::vector<std::vector<int>>;
using StateGrid = std::vector<std::vector<SolverCellState>>;
using ParentGrid = std::vector<std::vector<MazeDomain::CellId>>;

struct SolveExecutionState {
  ISearchEventSink* sink = nullptr;
  SolveOptions options;
  uint64_t next_seq = 1;
  int step_counter = 0;
  bool has_emitted_state = false;
  StateGrid last_emitted_states;
};

struct GridSize {
  int height;
  int width;
};

inline constexpr int kWallCount = MazeDomain::kWallCount;
inline constexpr int kInvalidCoord = -1;
inline constexpr GridPosition kInvalidCell = {kInvalidCoord, kInvalidCoord};
inline constexpr MazeDomain::CellId kInvalidCellId = MazeDomain::kInvalidCellId;
inline constexpr int kMaxCostDivisor = 4;

struct PathEndpoints {
  GridPosition start;
  GridPosition end;
};

struct PositionPair {
  GridPosition first;
  GridPosition second;
};

enum class NeighborOrder { VerticalFirst, Clockwise };

struct SearchTargets {
  GridPosition current;
  GridPosition end;
};

struct AStarNode {
  int f_score;
  int g_score;
  GridPosition pos;
};

struct AStarNodeCompare {
  auto operator()(const AStarNode& left, const AStarNode& right) const -> bool {
    if (left.f_score != right.f_score) {
      return left.f_score > right.f_score;
    }
    return left.g_score > right.g_score;
  }
};

auto GetGridSize(const MazeGrid& maze_grid) -> std::optional<GridSize>;
auto IsValidPosition(GridPosition pos, GridSize grid_size) -> bool;
auto CreateBoolGrid(GridSize grid_size, bool initial) -> BoolGrid;
auto CreateIntGrid(GridSize grid_size, int initial) -> IntGrid;
auto CreateStateGrid(GridSize grid_size, SolverCellState initial) -> StateGrid;
auto CreateParentGrid(GridSize grid_size, MazeDomain::CellId initial)
    -> ParentGrid;
auto ToCellId(GridSize grid_size, GridPosition pos) -> MazeDomain::CellId;
auto ToGridPosition(GridSize grid_size, MazeDomain::CellId cell_id)
    -> GridPosition;
auto CreateExecutionState(ISearchEventSink* sink, const SolveOptions& options)
    -> SolveExecutionState;
auto ShouldCancel(const SolveExecutionState& execution_state) -> bool;
void EmitRunStarted(SolveExecutionState& execution_state);
void EmitRunFailed(SolveExecutionState& execution_state,
                   std::string_view message);
void EmitRunCancelled(SolveExecutionState& execution_state,
                      std::string_view message);
void EmitRunFinished(SolveExecutionState& execution_state,
                     const SearchResult& result);
void EmitProgress(SolveExecutionState& execution_state,
                  const StateGrid& visual_states,
                  const std::vector<GridPosition>& current_path,
                  bool force_emit = false);
auto ShouldEmitProgressForCurrent(const ParentGrid& parents, GridPosition current,
                                  const PathEndpoints& endpoints) -> bool;
auto ShouldEmitBacktrackProgress(const ParentGrid& parents, GridPosition current,
                                 const PathEndpoints& endpoints) -> bool;
auto CreateTrivialResult(GridSize grid_size, GridPosition node,
                         SolveExecutionState& execution_state) -> SearchResult;
void FinalizeSearchResult(bool found, const PathEndpoints& endpoints,
                          const ParentGrid& parents, StateGrid& visual_states,
                          BoolGrid&& visited,
                          SolveExecutionState& execution_state,
                          SearchResult& result);
auto ManhattanDistance(PositionPair positions) -> int;
auto ReachableNeighbors(const MazeGrid& maze_grid, GridSize grid_size,
                        GridPosition current, NeighborOrder order)
    -> std::vector<GridPosition>;

}  // namespace MazeSolverDomain::detail

#endif  // MAZE_DOMAIN_MAZE_SOLVER_COMMON_H
