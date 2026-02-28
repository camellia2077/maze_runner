#ifndef MAZE_DOMAIN_GRID_TOPOLOGY_H
#define MAZE_DOMAIN_GRID_TOPOLOGY_H

#include <optional>
#include <vector>

namespace MazeDomain {

using CellId = int;

inline constexpr CellId kInvalidCellId = -1;
inline constexpr int kRowMajorBase = 1;
inline constexpr int kNeighborCount2D = 4;

enum class MazeDimension { D2 = 2, D3 = 3 };

struct Position2D {
  int row;
  int col;
};

struct GridSize2D {
  int height;
  int width;
};

class INeighborProvider {
 public:
  virtual ~INeighborProvider() = default;
  [[nodiscard]] virtual auto ReachableNeighbors(CellId cell_id) const
      -> std::vector<CellId> = 0;
};

class IGridTopology {
 public:
  virtual ~IGridTopology() = default;
  [[nodiscard]] virtual auto Dimension() const -> MazeDimension = 0;
  [[nodiscard]] virtual auto CellCount() const -> int = 0;
  [[nodiscard]] virtual auto IsValidCell(CellId cell_id) const -> bool = 0;
  [[nodiscard]] virtual auto AdjacentCells(CellId cell_id) const
      -> std::vector<CellId> = 0;
};

class GridTopology2D final : public IGridTopology {
 public:
  explicit GridTopology2D(GridSize2D grid_size);

  [[nodiscard]] auto Dimension() const -> MazeDimension override;
  [[nodiscard]] auto CellCount() const -> int override;
  [[nodiscard]] auto IsValidCell(CellId cell_id) const -> bool override;
  [[nodiscard]] auto AdjacentCells(CellId cell_id) const
      -> std::vector<CellId> override;

  [[nodiscard]] auto IsValidPosition(Position2D pos) const -> bool;
  [[nodiscard]] auto IndexFor(Position2D pos) const -> std::optional<CellId>;
  [[nodiscard]] auto PositionFor(CellId cell_id) const
      -> std::optional<Position2D>;
  [[nodiscard]] auto Size() const -> GridSize2D;

 private:
  GridSize2D grid_size_;
};

}  // namespace MazeDomain

#endif  // MAZE_DOMAIN_GRID_TOPOLOGY_H
