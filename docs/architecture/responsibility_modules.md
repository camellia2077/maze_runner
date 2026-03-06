# Responsibility Refactor Modules

## Win GUI (`apps/maze_win_gui/src`)

- `event/maze_event_parser.*`
  - Parse event JSON payload (`deltas/path/message/topology snapshot`) into plain data.
  - No Win32 dependency.
- `state/gui_state_store.*`
  - Own run state transitions, event application, cancel markers, paint snapshots.
- `render/maze_rasterizer.*`
  - Convert state snapshot to BGRA frame.
  - No direct control/window access.
- `runtime/session_runner.*`
  - Bridge Maze C API session lifecycle (`Create/Configure/Run/Cancel/Destroy`).
  - Forward callback events into state store + rasterizer.
- `main.cpp`
  - Keep assembly responsibilities only (`WinMain`, window/control wiring, message loop).

## Domain Generation (`libs/maze_core/domain`)

- `maze_generation_grid_ops.*`
  - Shared grid helpers (`Carve`, adjacency, bounds, visited grid, wall fill).
- `maze_generation_dfs.cpp`
- `maze_generation_prims.cpp`
- `maze_generation_kruskal.cpp`
- `maze_generation_recursive_division.cpp`
- `maze_generation_growing_tree.cpp`
  - One algorithm per file.
- `maze_generation_factory.cpp`
  - Generator registration, name mapping, parse helpers.
- `maze_generation.cpp`
  - Stable façade entry only.

## Domain Solver (`libs/maze_core/domain`)

- `maze_solver_grid_utils.*`
  - Grid size, coordinate conversion, grid creation, validation.
- `maze_solver_neighbors.*`
  - Reachable neighbors strategy by wall/opening + traversal order.
- `maze_solver_event_emitter.*`
  - Run events, progress frame emit, deltas/statistics.
- `maze_solver_result_builder.*`
  - Trivial result, finalize flow, path backtracking and path event emission.
- `maze_solver_{bfs,dfs,astar,dijkstra,greedy}.cpp`
  - Algorithm flow only, assembled from utility modules.

## Infra Graphics (`libs/maze_infra/graphics`)

- `render_layout.*`
  - Unit/pixel layout and geometric mapping.
- `render_palette.*`
  - Color policy by state/start/end/path/wall.
- `render_raster.*`
  - Raster painting for cells/walls/path points/corridors.
- `render_output_png.*`
  - PNG output adapter (directory ensure + `stb_image_write`).
- `maze_renderer.cpp`
  - Lightweight façade.
