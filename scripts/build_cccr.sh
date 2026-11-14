#!/usr/bin/env bash
set -euo pipefail
PIPELINE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ENGINE_DIR="$PIPELINE_DIR/cccr_engine"
OUTPUT_DIR="${1:-$PIPELINE_DIR/dist/cccr}"
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR=$(cd "$OUTPUT_DIR" && pwd)
cd "$PIPELINE_DIR"
export PATH="$PIPELINE_DIR/.venv/bin:$PATH"
export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
BUILD_TMP=$(mktemp -d)
trap 'rm -rf "$BUILD_TMP"' EXIT
uv build --python .venv/bin/python --no-build-isolation --wheel \
  --out-dir "$BUILD_TMP" "$ENGINE_DIR"
# Bundle external shared libraries and remove dependencies on build directories.
WHEEL_PLATFORM=$(.venv/bin/python -c 'import platform; print("manylinux_" + platform.libc_ver()[1].replace(".", "_") + "_" + platform.machine())')
.venv/bin/python -m auditwheel repair --plat "$WHEEL_PLATFORM" \
  --wheel-dir "$OUTPUT_DIR" "$BUILD_TMP"/*.whl
.venv/bin/python - "$ENGINE_DIR" "$OUTPUT_DIR" <<'PY'
import hashlib
import json
import pathlib
import platform
import sys

source, output = map(pathlib.Path, sys.argv[1:])
digest = hashlib.sha256()
for path in sorted(source.rglob('*')):
    if not path.is_file() or set(path.relative_to(source).parts) & {'.git', '.cache', 'build', 'bin', '__pycache__'}:
        continue
    digest.update(path.relative_to(source).as_posix().encode())
    digest.update(path.read_bytes())
manifest = {
    'cccr_version': '0.1.0', 'source_sha256': digest.hexdigest(),
    'python': platform.python_version(), 'platform': platform.platform(),
    'saxon': '13.0.0',
    'uv_lock_sha256': hashlib.sha256(pathlib.Path('uv.lock').read_bytes()).hexdigest(),
    'wheels': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in output.glob('*.whl')},
}
(output / 'build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY
