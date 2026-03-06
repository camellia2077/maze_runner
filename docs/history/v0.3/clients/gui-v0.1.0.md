# GUI Changelog - v0.1.0

Date: `2026-02-28`
Layer: `Windows GUI Presentation (apps/maze_win_gui)`
Depends on Core: `0.3.0`

## Highlights
- Added Windows GUI shell app target: `maze_win_gui`.
- Added parameter panel for maze size/start/end/unit pixels/output settings.
- Added algorithm selection with dropdowns for generation and search.
- Added async run model to keep UI responsive during execution.
- Added real-time frame rendering via callback (`RGBA -> BGRA` + `StretchDIBits`).
- Added cancellation path (`Cancel` button -> `MazeCancel(...)`).
- Added bounds safety for coordinates:
  - `Width/Height` changes auto-adjust `End X/End Y`
  - coordinate fields auto-clamp on focus loss
- Added GUI version identity:
  - `apps/maze_win_gui/common/version.hpp` => `0.1.0`
  - window title shows `gui=<version>, kernel=<version>`
- Added GUI open-source notice document:
  - `apps/maze_win_gui/OPEN_SOURCE_LICENSES.md`

## Versioning
- GUI version source: `apps/maze_win_gui/common/version.hpp`
- Core version source: generated from `libs/maze_core/VERSION`
