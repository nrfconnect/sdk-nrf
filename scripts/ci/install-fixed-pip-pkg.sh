#!/usr/bin/env bash
#
# Bootstrap pip from scripts/tools-versions-*.yml, then install pinned package
# versions from scripts/requirements-fixed.txt.
#
# Each positional argument is a grep(1) ERE fragment OR'd against requirement
# names, e.g.:
#
#   ./scripts/ci/install-fixed-pip-pkg.sh west
#   ./scripts/ci/install-fixed-pip-pkg.sh \
#       'setuptools|python-magic=|junitparser|lxml|gitlint|pylint|reuse|west'
#
set -euo pipefail

NRF_SCRIPTS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REQ_FILE="${NRF_SCRIPTS}/requirements-fixed.txt"
PYTHON="${PYTHON:-python3}"
PIP="${PIP:-pip3}"

pip_args=(install --only-binary :all: -U)
user=false
extra_index_url=""

usage() {
  echo "Usage: $0 [--user] [--extra-index-url URL] [--req-file PATH] PATTERN..." >&2
  exit 1
}

tools_versions_file() {
  case "$(uname -s)" in
    MINGW*) echo "${NRF_SCRIPTS}/tools-versions-win10.yml" ;;
    Linux*) echo "${NRF_SCRIPTS}/tools-versions-linux.yml" ;;
    Darwin*) echo "${NRF_SCRIPTS}/tools-versions-darwin.yml" ;;
    *) echo "Unsupported OS: $(uname -s)" >&2; return 1 ;;
  esac
}

pip_version_from_tools_versions() {
  local tools_versions="$1"
  awk '/^pip:/{found=1} found && /^[[:space:]]+version:/{print $2; exit}' \
    "$tools_versions"
}

install_pip_from_tools_versions() {
  local tools_versions pip_version
  tools_versions="$(tools_versions_file)"
  pip_version="$(pip_version_from_tools_versions "$tools_versions")"

  if [[ -z "$pip_version" ]]; then
    echo "No pip.version in $tools_versions" >&2
    exit 1
  fi

  local bootstrap_args=(--only-binary :all: -U "pip==${pip_version}")
  if $user; then
    bootstrap_args+=(--user)
  fi
  if [[ -n "$extra_index_url" ]]; then
    bootstrap_args+=(--extra-index-url "$extra_index_url")
  fi

  "$PYTHON" -m pip "${bootstrap_args[@]}"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --user)
      user=true
      shift
      ;;
    --extra-index-url)
      extra_index_url="$2"
      pip_args+=(--extra-index-url "$2")
      shift 2
      ;;
    --req-file)
      REQ_FILE="$2"
      shift 2
      ;;
    -h|--help)
      usage
      ;;
    --)
      shift
      break
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage
      ;;
    *)
      break
      ;;
  esac
done

[[ $# -gt 0 ]] || usage

if $user; then
  pip_args+=(--user)
fi

install_pip_from_tools_versions

pattern=$(printf '%s|' "$@")
pattern="${pattern%|}"
pattern="^(${pattern})"

mapfile -t specs < <(
  grep -E "$pattern" "$REQ_FILE" | cut -f1 -d'#' | awk '{print $1}'
)

if [[ ${#specs[@]} -eq 0 ]]; then
  echo "No packages matched pattern '$pattern' in $REQ_FILE" >&2
  exit 1
fi

exec "$PIP" "${pip_args[@]}" "${specs[@]}"
