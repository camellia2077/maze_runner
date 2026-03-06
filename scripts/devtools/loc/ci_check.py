import argparse
from dataclasses import dataclass
from pathlib import Path

from internal.config import load_language_config
from internal.service import LocScanService


@dataclass(frozen=True)
class GateRule:
    path: str
    max_lines: int


CPP_GATE_RULES: tuple[GateRule, ...] = (
    GateRule(path="apps/maze_win_gui/src", max_lines=650),
    GateRule(path="apps/maze_win_gui/src/event", max_lines=250),
    GateRule(path="apps/maze_win_gui/src/state", max_lines=300),
    GateRule(path="apps/maze_win_gui/src/render", max_lines=250),
    GateRule(path="apps/maze_win_gui/src/runtime", max_lines=250),
    GateRule(path="libs/maze_core/domain", max_lines=300),
    GateRule(path="libs/maze_infra/graphics", max_lines=250),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run LOC gate checks for key C++ directories."
    )
    parser.add_argument(
        "--config",
        default=str(Path(__file__).resolve().parent / "scan_lines.toml"),
        help="Path to scan_lines.toml",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[3]
    config = load_language_config(Path(args.config).resolve(), "cpp")
    scan_service = LocScanService(config)

    has_violation = False
    print("[LOC_GATE] checking C++ key directories...")
    for rule in CPP_GATE_RULES:
        target = (repo_root / rule.path).resolve()
        if not target.exists():
            print(f"[LOC_GATE][FAIL] missing path: {target}")
            has_violation = True
            continue

        matched = scan_service.analyze_path(target, mode="over", threshold=rule.max_lines)
        if matched:
            has_violation = True
            print(
                f"[LOC_GATE][FAIL] {rule.path}: found {len(matched)} files > {rule.max_lines} lines"
            )
            for file_path, lines in matched:
                print(f"  - {lines} | {file_path}")
        else:
            print(f"[LOC_GATE][PASS] {rule.path}: <= {rule.max_lines}")

    if has_violation:
        print("[LOC_GATE] overall=FAIL")
        return 1

    print("[LOC_GATE] overall=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
