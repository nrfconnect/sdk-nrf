#!/usr/bin/env bash
# Regenerate nrf/scripts/requirements-fixed.txt in a Linux/Python 3.12 container.
#
# From the west workspace root:
#   ./nrf/scripts/pip/run-pip-compile.sh
#
# From the nrf repo root:
#   ./scripts/pip/run-pip-compile.sh
#
# Options:
#   --shell   interactive shell instead of compile (prints SUCCESS/FAILURE on default run)
#
# Environment:
#   FORCE_REBUILD=1     rebuild the Docker image even if it already exists
#   NORDIC_PYPI_INDEX   PyPI index URL (for authenticated Artifactory access)
#   WORKSPACE           west topdir (auto-detected if unset)
#
# Colima note: --dns is required because the default VM DNS often fails inside containers.
#
set -euo pipefail

IMAGE="${IMAGE:-nrf-pip-compile:3.12}"
CACHE_VOLUME="${CACHE_VOLUME:-nrf-pip-compile-cache}"
NORDIC_PYPI_INDEX="${NORDIC_PYPI_INDEX:-https://files.nordicsemi.com/artifactory/api/pypi/nordic-pypi/simple}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# West topdir: parent of nrf/ (bootloader, zephyr, nrf, ...)
WORKSPACE="${WORKSPACE:-$(cd "${SCRIPT_DIR}/../../.." && pwd)}"

log() {
    printf '[run-pip-compile] %s\n' "$*"
}

SHELL_MODE=false
if [[ "${1:-}" == "--shell" ]]; then
    SHELL_MODE=true
    shift
fi

if [[ $# -gt 0 ]]; then
    echo "usage: $0 [--shell]" >&2
    exit 2
fi

for dir in bootloader zephyr nrf; do
    if [[ ! -d "${WORKSPACE}/${dir}" ]]; then
        echo "error: ${WORKSPACE} does not look like a west workspace (missing ${dir}/)" >&2
        echo "Set WORKSPACE to your west topdir and retry." >&2
        exit 1
    fi
done

read -r -d '' COMPILE_CMD <<'EOF' || true
pip-compile-cross-platform \
  bootloader/mcuboot/scripts/requirements.txt \
  zephyr/scripts/requirements.txt \
  nrf/scripts/requirements-ci.txt \
  nrf/scripts/requirements-extra.txt \
  nrf/scripts/requirements.txt \
  --output-file nrf/scripts/requirements-fixed.txt \
  --min-python-version 3.12 \
  --index-url "${NORDIC_PYPI_INDEX}"
EOF

docker_run_args=(
    --rm
    --platform linux/amd64
    --dns 8.8.8.8
    --dns 1.1.1.1
    -e PYTHONUNBUFFERED=1
    -e "NORDIC_PYPI_INDEX=${NORDIC_PYPI_INDEX}"
    -v "${WORKSPACE}:/work"
    -v "${CACHE_VOLUME}:/opt/pip-compile-cache"
    -w /work
)

if [[ "${FORCE_REBUILD:-0}" == 1 ]] || ! docker image inspect "${IMAGE}" >/dev/null 2>&1; then
    log "Building image ${IMAGE}..."
    DOCKER_BUILDKIT="${DOCKER_BUILDKIT:-0}" docker build --pull=false \
        --platform linux/amd64 \
        -f "${SCRIPT_DIR}/Dockerfile.pip-compile" \
        -t "${IMAGE}" \
        "${SCRIPT_DIR}"
else
    log "Using existing image ${IMAGE} (set FORCE_REBUILD=1 to rebuild)."
fi

docker volume create "${CACHE_VOLUME}" >/dev/null

if [[ "${SHELL_MODE}" == true ]]; then
    exec docker run -it "${docker_run_args[@]}" "${IMAGE}"
fi

OUTPUT_FILE="${WORKSPACE}/nrf/scripts/requirements-fixed.txt"
OLD_FILE="$(mktemp)"
compile_started=false

cleanup() {
    if [[ "${compile_started}" == true && ! -f "${OUTPUT_FILE}" && -f "${OLD_FILE}" ]]; then
        cp "${OLD_FILE}" "${OUTPUT_FILE}"
    fi
    rm -f "${OLD_FILE}"
}
trap cleanup EXIT

cp "${OUTPUT_FILE}" "${OLD_FILE}" 2>/dev/null || : >"${OLD_FILE}"
rm -f "${OUTPUT_FILE}"

log "Compiling requirements (often 2-5 minutes, little output while resolving)..."
compile_started=true
set +e
docker run "${docker_run_args[@]}" "${IMAGE}" -c "${COMPILE_CMD}"
status=$?
set -e
compile_started=false

if [[ ${status} -ne 0 ]]; then
    cp "${OLD_FILE}" "${OUTPUT_FILE}"
    echo "FAILURE"
    exit "${status}"
fi

echo "--- requirements-fixed.txt diff ---"
diff_output="$(diff -u "${OLD_FILE}" "${OUTPUT_FILE}" || true)"
if [[ -z "${diff_output}" ]]; then
    echo "(no changes)"
else
    printf '%s\n' "${diff_output}"
fi
echo "SUCCESS"
exit 0
