# Core Changelog - v0.3.0

Date: `2026-02-28`
Layer: `Kernel (maze_core + maze_usecase + maze_infra + maze_api_c)`

## Highlights
- Introduced topology-driven architecture baseline: `CellId`, `IGridTopology`, `GridTopology2D`, `MazeRuntimeContext`.
- Migrated solver algorithms (BFS/DFS/A*/Dijkstra/Greedy) to neighbor-iteration flow; removed fixed 2D delta coupling.
- Migrated generation algorithms (DFS/Prim/Kruskal/GrowingTree) to topology-driven flow; kept Recursive Division as 2D-only.
- Upgraded config/runtime orchestration: supports `dimension/depth/start_z/end_z` parsing and validation; 3D path is reserved (not executed yet).
- Split build targets into `maze_core`, `maze_infra`, `maze_usecase`, `maze_api_c`, `maze_cli`.
- Added package install/export (`MazeGeneratorTargets`, `MazeGeneratorConfig.cmake`) and consumer smoke coverage.
- Expanded tests for runtime/config/CLI/API behavior, and kept build + ctest green through migration.

## Versioning
- Core version source: `libs/maze_core/VERSION`
- Generated header: `build/generated/common/kernel_version.hpp`
