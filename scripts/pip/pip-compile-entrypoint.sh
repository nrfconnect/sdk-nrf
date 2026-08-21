#!/usr/bin/env bash
set -euo pipefail

NORDIC_PYPI_INDEX="${NORDIC_PYPI_INDEX:-https://files.nordicsemi.com/artifactory/api/pypi/nordic-pypi/simple}"
CACHE_DIR="${PIP_COMPILE_CACHE_DIR:-/opt/pip-compile-cache}"
VENV="${CACHE_DIR}/venv"
MARKER="${VENV}/.installed"

log() {
    printf '[pip-compile] %s\n' "$*"
}

if [[ ! -x "${VENV}/bin/pip-compile-cross-platform" ]]; then
    log "Installing pip-compile-cross-platform into ${VENV} (first run only)..."
    rm -rf "${VENV}"
    python3 -m venv "${VENV}"
    "${VENV}/bin/pip" install --no-cache-dir --upgrade pip
    "${VENV}/bin/pip" install --no-cache-dir \
        --index-url "${NORDIC_PYPI_INDEX}" \
        pip-compile-cross-platform==1.4.2+nordic.3
    touch "${MARKER}"
    log "Install complete."
else
    log "Using cached tool install in ${VENV}."
fi

export PATH="${VENV}/bin:${PATH}"

if [[ $# -eq 0 ]]; then
    exec /bin/bash
elif [[ "$1" == "-c" || "$1" == "-lc" ]]; then
    log "Running: ${2:-}"
    exec /bin/bash "$@"
else
    log "Running: $*"
    exec "$@"
fi
