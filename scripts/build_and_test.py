import argparse
import subprocess
import sys
from pathlib import Path


def run_step(name: str, command: list[str], working_dir: Path) -> int:
    print(f"[STEP] {name}", flush=True)
    print(f"[CMD] {' '.join(command)}", flush=True)
    result = subprocess.run(command, cwd=str(working_dir))
    print(f"[RC] {name}={result.returncode}", flush=True)
    return result.returncode


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build project and run tests in one command.")
    parser.add_argument(
        "--build-dir",
        default="build",
        help="CMake build directory, must match build.py output directory.",
    )
    parser.add_argument(
        "build_args",
        nargs=argparse.REMAINDER,
        help="Arguments forwarded to scripts/build.py. Example: -- --build-type Debug --no-opt",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    project_root = Path(__file__).resolve().parent.parent
    build_py = project_root / "scripts" / "build.py"
    build_dir = project_root / args.build_dir

    forwarded_args = list(args.build_args)
    if forwarded_args and forwarded_args[0] == "--":
        forwarded_args = forwarded_args[1:]

    has_build_dir_flag = any(
        arg == "--build-dir" or arg.startswith("--build-dir=") for arg in forwarded_args
    )
    if not has_build_dir_flag:
        forwarded_args = ["--build-dir", args.build_dir, *forwarded_args]

    build_cmd = [sys.executable, str(build_py), *forwarded_args]
    build_rc = run_step("build", build_cmd, project_root)
    if build_rc != 0:
        print("[SUMMARY] build=FAIL test=SKIP overall=FAIL")
        return build_rc

    test_cmd = ["ctest", "--test-dir", str(build_dir), "--output-on-failure"]
    test_rc = run_step("test", test_cmd, project_root)
    if test_rc != 0:
        print("[SUMMARY] build=PASS test=FAIL overall=FAIL")
        return test_rc

    print("[SUMMARY] build=PASS test=PASS overall=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
