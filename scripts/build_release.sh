#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PYTHON_BIN="python"
if ! command -v "${PYTHON_BIN}" >/dev/null 2>&1; then
  PYTHON_BIN="python3"
fi

"${PYTHON_BIN}" "${SCRIPT_DIR}/build.py" \
  --build-type Release \
  --build-dir build_release \
  --opt \
  --release-opt-level O2 \
  --release-extra-flags "" \
  --strip \
  --static-link
