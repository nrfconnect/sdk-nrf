#!/usr/bin/env bash
# Regenerate nrf/scripts/requirements-fixed.txt using Docker (linux/amd64, Python
# 3.12.4), matching nrf/scripts/tools-versions-linux.yml.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: requirements-fixed.sh [-i|--interactive]

  Default: upgrade pip, install pip-compile-cross-platform, run pip-compile.

  -i, --interactive
      Same setup, then start an interactive bash shell in /ncs (does not run
      pip-compile-cross-platform). Run it manually when ready.

  Run from the west workspace root, or from nrf/scripts (paths are resolved).

  Environment: OUT_FILE, INDEX_URL, PYTHON_VERSION, PIP_COMPILE_DOCKER_IMAGE,
               DOCKER_PLATFORM
EOF
}

INTERACTIVE=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    -i | --interactive)
      INTERACTIVE=1
      shift
      ;;
    -h | --help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

BASEDIR=$(cd "$(dirname "$0")" && pwd)
NCS_TOP=$(cd "$BASEDIR/../.." && pwd)
OUT_FILE="${OUT_FILE:-nrf/scripts/requirements-fixed.txt}"
INDEX_URL="${INDEX_URL:-https://files.nordicsemi.com/artifactory/api/pypi/nordic-pypi/simple}"

PYTHON_VERSION="${PYTHON_VERSION:-3.12.4}"
IMAGE="${PIP_COMPILE_DOCKER_IMAGE:-python:${PYTHON_VERSION}}"

DOCKER_PLATFORM="${DOCKER_PLATFORM:-linux/amd64}"

echo "NCS workspace: $NCS_TOP"
echo "Docker image:  $IMAGE (platform: $DOCKER_PLATFORM)"
echo "Output file:   $OUT_FILE"
if [[ "$INTERACTIVE" == 1 ]]; then
  echo "Mode:          interactive (setup only, then shell)"
fi

DOCKER_ARGS=(
  --rm
  --platform "$DOCKER_PLATFORM"
  --entrypoint /bin/bash
  -v "$NCS_TOP:/ncs"
  -w /ncs
  -e OUT_FILE="$OUT_FILE"
  -e INDEX_URL="$INDEX_URL"
  -e INTERACTIVE="$INTERACTIVE"
)

if [[ "$INTERACTIVE" == 1 ]]; then
  DOCKER_ARGS+=(-it)
fi

exec docker run "${DOCKER_ARGS[@]}" "$IMAGE" -lc '
  set -euo pipefail
  echo "Python: $(python --version) ($(which python))"

  python -m pip install -U pip setuptools wheel
  pip install --index-url "$INDEX_URL" \
    "pip-compile-cross-platform==1.4.2+nordic.3" --upgrade

  if [[ "$INTERACTIVE" == 1 ]]; then
    cat <<EOF

Setup done. Example pip-compile (not run automatically):

  pip-compile-cross-platform \\
    bootloader/mcuboot/scripts/requirements.txt \\
    zephyr/scripts/requirements.txt \\
    nrf/scripts/requirements-ci.txt \\
    nrf/scripts/requirements-extra.txt \\
    nrf/scripts/requirements.txt \\
    --output-file "$OUT_FILE" \\
    --min-python-version 3.12 \\
    --index-url "$INDEX_URL"

EOF
    exec bash
  fi

  pip-compile-cross-platform \
    bootloader/mcuboot/scripts/requirements.txt \
    zephyr/scripts/requirements.txt \
    nrf/scripts/requirements-ci.txt \
    nrf/scripts/requirements-extra.txt \
    nrf/scripts/requirements.txt \
    --output-file "$OUT_FILE" \
    --min-python-version 3.12 \
    --index-url "$INDEX_URL"
'
