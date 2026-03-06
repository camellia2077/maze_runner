# Dependency Directions

## Primary Direction Rules

- UI side: `ui(main)` -> `runtime` -> `api`
- GUI parse/state/render modules must not depend on Win32-specific control APIs.
- Domain modules (`maze_core/domain`) do not reverse-depend on infra or UI.
- Infra renderer consumes domain/application outputs only.

## Refactor Dependency Graph (ASCII)

```text
apps/maze_win_gui/src/main.cpp
  -> runtime/session_runner
    -> api/maze_api.h (C API)
    -> state/gui_state_store
      -> event/maze_event_parser
      -> render/maze_rasterizer

libs/maze_core/domain/maze_solver_{bfs,dfs,astar,dijkstra,greedy}
  -> maze_solver_grid_utils
  -> maze_solver_neighbors
  -> maze_solver_event_emitter
  -> maze_solver_result_builder

libs/maze_core/domain/maze_generation
  -> maze_generation_factory
  -> maze_generation_{dfs,prims,kruskal,recursive_division,growing_tree}
    -> maze_generation_grid_ops

libs/maze_infra/graphics/maze_renderer
  -> render_raster
    -> render_layout
    -> render_palette
  -> render_output_png
```
