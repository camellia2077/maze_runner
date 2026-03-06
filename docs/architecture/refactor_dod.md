# Refactor DoD Checklist

## Scope

- Win GUI responsibility split
- Maze generation split
- Maze solver common split
- Infra renderer split
- Governance gates and module tests

## Done Criteria

- LOC gate command is available:
  - `python scripts/devtools/loc/ci_check.py`
- New module-level tests are integrated into `maze_unit_tests`:
  - parser / state / rasterizer / event_emitter / result_builder
- Module responsibility and dependency direction docs are recorded:
  - `docs/architecture/responsibility_modules.md`
  - `docs/architecture/dependency_directions.md`
- Unified validation command passes:
  - `python scripts/build_and_test.py`

## Notes

- LOC target uses warning thresholds on key directories.
- Recommended long-file target remains `< 300` lines per file when practical.
