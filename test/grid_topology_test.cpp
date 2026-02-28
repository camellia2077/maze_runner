#include <iostream>
#include <optional>
#include <vector>

#include "domain/grid_topology.h"

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

}  // namespace

auto RunGridTopologyTests() -> int {
  int failures = 0;
  const MazeDomain::GridTopology2D topology(
      MazeDomain::GridSize2D{.height = 3, .width = 4});

  ExpectEqual(topology.Dimension(), MazeDomain::MazeDimension::D2,
              "Topology dimension should be 2D", failures);
  ExpectEqual(topology.CellCount(), 12, "Cell count should be 12", failures);

  ExpectTrue(topology.IsValidCell(0), "Cell 0 should be valid", failures);
  ExpectTrue(topology.IsValidCell(11), "Cell 11 should be valid", failures);
  ExpectTrue(!topology.IsValidCell(-1), "Cell -1 should be invalid", failures);
  ExpectTrue(!topology.IsValidCell(12), "Cell 12 should be invalid", failures);

  const std::optional<MazeDomain::CellId> center_cell =
      topology.IndexFor(MazeDomain::Position2D{.row = 1, .col = 1});
  ExpectTrue(center_cell.has_value(), "Center position should map to index",
             failures);
  if (center_cell.has_value()) {
    ExpectEqual(*center_cell, 5, "Center index should be 5", failures);
  }

  const std::optional<MazeDomain::Position2D> center_pos =
      topology.PositionFor(5);
  ExpectTrue(center_pos.has_value(), "Index 5 should map to position", failures);
  if (center_pos.has_value()) {
    ExpectEqual(center_pos->row, 1, "Index 5 row should be 1", failures);
    ExpectEqual(center_pos->col, 1, "Index 5 col should be 1", failures);
  }

  const std::vector<MazeDomain::CellId> center_neighbors =
      topology.AdjacentCells(5);
  const std::vector<MazeDomain::CellId> expected_center_neighbors = {1, 6, 9,
                                                                      4};
  ExpectEqual(center_neighbors, expected_center_neighbors,
              "Center neighbors should be up,right,down,left", failures);

  const std::vector<MazeDomain::CellId> corner_neighbors =
      topology.AdjacentCells(0);
  const std::vector<MazeDomain::CellId> expected_corner_neighbors = {1, 4};
  ExpectEqual(corner_neighbors, expected_corner_neighbors,
              "Corner neighbors should contain two entries", failures);

  return failures;
}
