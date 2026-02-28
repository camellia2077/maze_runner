#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
python "${SCRIPT_DIR}/build.py" --target tidy --tidy-mode check --build-type Debug --no-opt --build-dir build_tidy
