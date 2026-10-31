#!/usr/bin/env bash
# Regenerate nrf/scripts/requirements-fixed.txt in Docker (linux/amd64, Python
# 3.12.4), matching nrf/scripts/tools-versions-linux.yml.
# Do not enable nounset when sourced (CI uses virtualenvwrapper in the same shell).
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -euo pipefail
fi

BASEDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REQUIREMENTS_EXCLUDE="${REQUIREMENTS_EXCLUDE:-nrf/scripts/requirements-exclude.txt}"
REQUIREMENTS_FIXED_IN="${REQUIREMENTS_FIXED_IN:-nrf/scripts/requirements-fixed.in}"

declare -A FLATTEN_VISITED=()
declare -a EXCLUDED=()

load_excludes() {
  local line
  EXCLUDED=()
  [[ -f "$NCS_TOP/$REQUIREMENTS_EXCLUDE" ]] || return 0
  while IFS= read -r line || [[ -n "$line" ]]; do
    line="${line%%#*}"
    line="${line// /}"
    [[ -z "$line" ]] && continue
    EXCLUDED+=("$(echo "$line" | tr '[:upper:]' '[:lower:]')")
  done <"$NCS_TOP/$REQUIREMENTS_EXCLUDE"
}

is_excluded() {
  local pkg=$1 lower ex
  pkg="${pkg%%[[:space:][]*}"
  lower=$(echo "$pkg" | tr '[:upper:]' '[:lower:]')
  for ex in "${EXCLUDED[@]}"; do
    [[ "$lower" == "$ex" ]] && return 0
  done
  return 1
}

resolve_under_top() {
  local base_dir=$1 ref=$2
  ref="${ref%%#*}"
  ref=$(echo "$ref" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
  if [[ "$ref" = /* ]]; then
    echo "$ref"
  else
    echo "$(cd "$base_dir" && cd "$(dirname "$ref")" && pwd)/$(basename "$ref")"
  fi
}

# Remove excluded package lines from one requirements file (-r includes visited first).
apply_excludes_to_file() {
  local src_abs=$1
  [[ -n "${FLATTEN_VISITED[$src_abs]+x}" ]] && return 0
  [[ -f "$src_abs" ]] || { echo "requirements file not found: $src_abs" >&2; return 1; }
  FLATTEN_VISITED[$src_abs]=1

  local base_dir line pkg ref ref_abs tmp
  base_dir=$(dirname "$src_abs")
  while IFS= read -r line || [[ -n "$line" ]]; do
    if [[ "$line" =~ ^[[:space:]]*-r[[:space:]]+ ]]; then
      ref=${line#*-r}
      ref=$(echo "$ref" | sed 's/^[[:space:]]*//')
      ref_abs=$(resolve_under_top "$base_dir" "$ref")
      apply_excludes_to_file "$ref_abs"
    fi
  done <"$src_abs"

  tmp=$(mktemp)
  while IFS= read -r line || [[ -n "$line" ]]; do
    if [[ "$line" =~ ^[[:space:]]*# ]] || [[ -z "${line//[[:space:]]/}" ]]; then
      echo "$line"
      continue
    fi
    if [[ "$line" =~ ^[[:space:]]*-r[[:space:]]+ ]]; then
      echo "$line"
      continue
    fi
    pkg=$(echo "$line" | awk '{print $1}')
    if is_excluded "$pkg"; then
      continue
    fi
    echo "$line"
  done <"$src_abs" >"$tmp"
  mv "$tmp" "$src_abs"
}

# Always run before pip-compile: requirements-fixed.in tree + requirements-exclude.txt.
apply_requirements_excludes() {
  local input
  FLATTEN_VISITED=()
  load_excludes
  while IFS= read -r input || [[ -n "$input" ]]; do
    [[ "$input" =~ ^[[:space:]]*# ]] && continue
    [[ -z "${input//[[:space:]]/}" ]] && continue
    apply_excludes_to_file "$NCS_TOP/$input"
  done <"$NCS_TOP/$REQUIREMENTS_FIXED_IN"
}

read_compile_inputs() {
  local input
  COMPILE_INPUTS=()
  while IFS= read -r input || [[ -n "$input" ]]; do
    [[ "$input" =~ ^[[:space:]]*# ]] && continue
    [[ -z "${input//[[:space:]]/}" ]] && continue
    COMPILE_INPUTS+=("$input")
  done <"$NCS_TOP/$REQUIREMENTS_FIXED_IN"
}

usage() {
  cat <<'EOF'
Usage: requirements-fixed.sh [-h|--help] [--check-index]

  Default: apply requirements-exclude.txt, then pip-compile-cross-platform.

  --check-index: each lock pin on the simple index with cp312 wheels for linux
  (manylinux x86_64), windows (win_amd64), and mac (arm64 or universal2); no pip cache.
  Prints SUCCESS or FAIL: <package> for missing pins.

  Source this script and call apply_requirements_excludes with NCS_TOP set (CI).

  Example: CONTAINER_CMD=podman ./requirements-fixed.sh

  Environment: OUT_FILE, INDEX_URL, PYTHON_VERSION, CONTAINER_CMD,
               REQUIREMENTS_FIXED_IN, REQUIREMENTS_EXCLUDE, NCS_TOP,
               PIP_CACHE_DIR (host path mounted for pip wheel/metadata cache),
               PIP_COMPILE_VERBOSE=1 (stream Poetry resolve / HTTP debug),
               PIP_COMPILE_CONTAINER_IMAGE (or PIP_COMPILE_DOCKER_IMAGE),
               CONTAINER_PLATFORM (or DOCKER_PLATFORM)
EOF
}

init_workspace() {
  NCS_TOP=${NCS_TOP:-$(cd "$BASEDIR/../.." && pwd)}
  NCS_TOP=$(cd "$NCS_TOP" && pwd)
  OUT_FILE="${OUT_FILE:-nrf/scripts/requirements-fixed.txt}"
  INDEX_URL="${INDEX_URL:-https://files.nordicsemi.com/artifactory/api/pypi/nordic-pypi/simple}"
  PYTHON_VERSION="${PYTHON_VERSION:-3.12.4}"
  IMAGE="${PIP_COMPILE_CONTAINER_IMAGE:-${PIP_COMPILE_DOCKER_IMAGE:-python:${PYTHON_VERSION}}}"
  CONTAINER_PLATFORM="${CONTAINER_PLATFORM:-${DOCKER_PLATFORM:-linux/amd64}}"
  CONTAINER_CMD="${CONTAINER_CMD:-docker}"
  CONTAINER_PIP_CACHE="/root/.cache/pip"
}

# container_run [-v host:path ...] [-e KEY=VAL ...] -- 'bash -lc script'
container_run() {
  local extra_volumes=() extra_env=() script=""
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -v)
        extra_volumes+=(-v "$2")
        shift 2
        ;;
      -e)
        extra_env+=(-e "$2")
        shift 2
        ;;
      --)
        shift
        script=$1
        break
        ;;
      *)
        echo "container_run: unknown argument: $1" >&2
        exit 1
        ;;
    esac
  done
  [[ -n "$script" ]] || {
    echo "container_run: missing script after --" >&2
    exit 1
  }

  "$CONTAINER_CMD" run \
    --rm \
    --platform "$CONTAINER_PLATFORM" \
    --entrypoint /bin/bash \
    -v "$NCS_TOP:/ncs" \
    "${extra_volumes[@]}" \
    -w /ncs \
    "${extra_env[@]}" \
    -e OUT_FILE="$OUT_FILE" \
    -e INDEX_URL="$INDEX_URL" \
    "$IMAGE" \
    -lc "$script"
}

run_regen() {
  init_workspace

  if [[ ! -f "$NCS_TOP/$REQUIREMENTS_FIXED_IN" ]]; then
    echo "compile input list not found: $NCS_TOP/$REQUIREMENTS_FIXED_IN" >&2
    exit 1
  fi

  apply_requirements_excludes
  read_compile_inputs
  compile_inputs_env=$(printf '%s\n' "${COMPILE_INPUTS[@]}")

  local host_pip_cache
  host_pip_cache="${PIP_CACHE_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/ncs-pip-compile}"
  mkdir -p "$host_pip_cache"

  echo "NCS workspace: $NCS_TOP"
  echo "Container:     $CONTAINER_CMD"
  echo "Image:         $IMAGE (platform: $CONTAINER_PLATFORM)"
  echo "Output file:   $OUT_FILE"
  echo "Compile list:  $REQUIREMENTS_FIXED_IN"
  echo "Exclude file:  $REQUIREMENTS_EXCLUDE"
  echo "Pip cache:     $host_pip_cache"
  echo "Verbose:       ${PIP_COMPILE_VERBOSE:-0}"

  container_run \
    -v "$host_pip_cache:$CONTAINER_PIP_CACHE" \
    -e "COMPILE_INPUTS=$compile_inputs_env" \
    -e "PIP_CACHE_DIR=$CONTAINER_PIP_CACHE" \
    -e "PIP_COMPILE_VERBOSE=${PIP_COMPILE_VERBOSE:-}" \
    -- "$(cat <<'REGEN_CONTAINER'
set -euo pipefail
echo "Python: $(python --version) ($(which python))"

python -m pip install -U pip setuptools wheel
pip install --index-url "$INDEX_URL" \
  "pip-compile-cross-platform==1.4.2+nordic.3" --upgrade

if [[ "${PIP_COMPILE_VERBOSE:-}" == 1 ]]; then
  export PYTHONUNBUFFERED=1
  export POETRY_HTTP_DEBUG=1
  python <<'PY'
import pathlib, sys
import pip_compile_cross_platform as pc

path = pathlib.Path(pc.__file__).with_name("__init__.py")
text = path.read_text()
old_run = """def _run_poetry(work_dir: Path, *args):
    env = os.environ.copy()
    env["POETRY_WARNINGS_EXPORT"] = "false"

    result = subprocess.run(
        [sys.executable, "-m", "poetry", *args],"""
new_run = """def _run_poetry(work_dir: Path, *args):
    env = os.environ.copy()
    env["POETRY_WARNINGS_EXPORT"] = "false"

    poetry_args = list(args)
    if os.environ.get("PIP_COMPILE_VERBOSE") == "1" and "export" not in poetry_args:
        poetry_args = ["-vvv", *poetry_args]

    result = subprocess.run(
        [sys.executable, "-m", "poetry", *poetry_args],"""
if old_run not in text:
    sys.exit("pip-compile-cross-platform verbose patch failed")
text = text.replace(old_run, new_run)
needle = (
    "    if result.returncode:\n"
    "        print(result.stdout, file=sys.stderr)\n"
    '        raise Exception("Poetry encountered an error")\n'
    "\n"
    "    return result"
)
insert = (
    '    if os.environ.get("PIP_COMPILE_VERBOSE") == "1" and result.stdout:\n'
    "        print(result.stdout, file=sys.stderr, flush=True)\n"
    + needle
)
if needle not in text:
    sys.exit("pip-compile-cross-platform verbose patch failed (needle)")
path.write_text(text.replace(needle, insert))
PY
  echo "Verbose: Poetry -vvv (not export) and POETRY_HTTP_DEBUG enabled"
fi

mapfile -t compile_inputs <<< "$COMPILE_INPUTS"
pip-compile-cross-platform \
  "${compile_inputs[@]}" \
  --output-file "$OUT_FILE" \
  --min-python-version 3.12 \
  --index-url "$INDEX_URL"
echo "SUCCESS: pip-compile wrote $OUT_FILE"
REGEN_CONTAINER
)"
}

run_check_index() {
  init_workspace
  [[ -f "$NCS_TOP/$OUT_FILE" ]] || { echo "FAIL: missing $OUT_FILE" >&2; exit 1; }

  echo "NCS workspace: $NCS_TOP"
  echo "Container:     $CONTAINER_CMD"
  echo "Image:         $IMAGE (platform: $CONTAINER_PLATFORM)"
  echo "Output file:   $OUT_FILE"
  echo "Index:         $INDEX_URL"
  echo "Pip cache:     (none — check-index)"
  local py_mm="${PYTHON_VERSION:-3.12.4}"
  py_mm="${py_mm%.*}"
  local check_py_tag="cp${py_mm//./}"
  echo "Wheel tags:    ${check_py_tag} on linux / windows / mac_arm"

  container_run \
    -e "CHECK_INDEX_PYTHON_TAG=${check_py_tag}" \
    -- "$(cat <<'CHECK_INDEX'
set -euo pipefail
python <<'PY'
import os, re, sys, urllib.error, urllib.request
from urllib.parse import urljoin

INDEX_URL = os.environ["INDEX_URL"].rstrip("/") + "/"
REQ_PATH = os.path.join("/ncs", os.environ["OUT_FILE"])
PYTHON_TAG = os.environ.get("CHECK_INDEX_PYTHON_TAG", "cp312")

# Match pip_requestor / toolchain bundler targets (linux, windows, mac_arm).
OS_VARIANTS = (
    ("linux", "manylinux x86_64"),
    ("windows", "win_amd64"),
    ("mac_arm", "macosx arm64 or universal2"),
)


def normalize(name):
    return re.sub(r"[-_.]+", "-", name).lower()


def skip_marker(marker):
    m = marker.strip()
    return 'platform_system == "Windows"' in m or 'sys_platform == "win32"' in m


def parse_spec(line):
    line = line.strip()
    if not line or line.startswith("#") or line.startswith("--"):
        return None
    req, _, marker = line.partition(";")
    if marker and skip_marker(marker):
        return None
    req = req.strip()
    m = re.match(r"^([a-zA-Z0-9][a-zA-Z0-9._-]*)(?:\[[^\]]+\])?==(.+)$", req)
    if not m:
        return None
    return normalize(m.group(1)), m.group(2).strip()


def http_status(url, method="GET"):
    req = urllib.request.Request(url, method=method)
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            if method == "GET":
                resp.read()
            return resp.status
    except urllib.error.HTTPError as e:
        return e.code


def wheel_filename(href):
    return href.split("/")[-1].split("#")[0]


def wheels_for_version(body, ver):
    # Artifactory/simple pages put #sha256 fragments after .whl before the closing quote.
    href_pat = re.compile(r'href=["\']([^"\']+\.whl)(?:#[^"\']*)?["\']', re.I)
    ver_needle = f"-{ver}-"
    out = []
    for m in href_pat.finditer(body):
        fn = wheel_filename(m.group(1))
        if ver_needle in fn:
            out.append(fn)
    return out


def compatible_python(fn):
    if "py3-none-any" in fn:
        return True
    if f"-{PYTHON_TAG}-" in fn:
        return True
    if "-abi3-" in fn and ("manylinux" in fn or "win_" in fn or "macosx" in fn):
        return True
    return False


def has_linux_wheel(wheels):
    for fn in wheels:
        if not compatible_python(fn):
            continue
        if "py3-none-any" in fn:
            return True
        if "manylinux" in fn and "x86_64" in fn:
            return True
    return False


def has_windows_wheel(wheels):
    for fn in wheels:
        if not compatible_python(fn):
            continue
        if "py3-none-any" in fn:
            return True
        if "win_amd64" in fn:
            return True
    return False


def has_mac_arm_wheel(wheels):
    for fn in wheels:
        if not compatible_python(fn):
            continue
        if "py3-none-any" in fn:
            return True
        if "universal2" in fn and "macosx" in fn:
            return True
        if "macosx" in fn and "arm64" in fn:
            return True
    return False


OS_WHEEL_FN = {
    "linux": has_linux_wheel,
    "windows": has_windows_wheel,
    "mac_arm": has_mac_arm_wheel,
}


seen = set()
missing = []
checked = 0
with open(REQ_PATH, encoding="utf-8") as f:
    for line in f:
        parsed = parse_spec(line)
        if not parsed:
            continue
        name, ver = parsed
        key = (name, ver)
        if key in seen:
            continue
        seen.add(key)
        checked += 1
        simple_url = INDEX_URL + name + "/"
        st = http_status(simple_url, "GET")
        if st != 200:
            missing.append(f"{name}=={ver} (simple index HTTP {st})")
            continue
        with urllib.request.urlopen(simple_url, timeout=120) as resp:
            body = resp.read().decode("utf-8", "replace")
        wheels = wheels_for_version(body, ver)
        if not wheels:
            missing.append(f"{name}=={ver} (no wheels on simple index)")
            continue
        absent = []
        for os_key, os_label in OS_VARIANTS:
            if not OS_WHEEL_FN[os_key](wheels):
                absent.append(os_label)
        if absent:
            missing.append(
                f"{name}=={ver} (missing wheel for {PYTHON_TAG}: {', '.join(absent)})"
            )
            continue
        sample_fn = wheels[0]
        for fn in wheels:
            if "manylinux" in fn and "x86_64" in fn:
                sample_fn = fn
                break
        hm = re.search(
            r'href=["\']([^"\']*'
            + re.escape(sample_fn)
            + r'(?:#[^"\']*)?)["\']',
            body,
            re.I,
        )
        if not hm:
            missing.append(f"{name}=={ver} (wheel link not found on simple index)")
            continue
        href = hm.group(1)
        wheel_url = href if href.startswith("http") else urljoin(simple_url, href)
        wstatus = http_status(wheel_url, "HEAD")
        if wstatus not in (200, 302, 303, 307, 308, 405):
            missing.append(f"{name}=={ver} (wheel HTTP {wstatus})")

for item in missing:
    print(f"FAIL: {item}")
if missing:
    print(
        f"FAIL: check-index {len(missing)} missing of {checked} checked",
        file=sys.stderr,
    )
    sys.exit(1)
print(f"SUCCESS: check-index {checked} pins on {INDEX_URL.rstrip('/')}")
PY
CHECK_INDEX
)"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  mode=regen
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -h | --help)
        usage
        exit 0
        ;;
      --check-index)
        mode=check-index
        shift
        ;;
      *)
        echo "Unknown option: $1" >&2
        usage >&2
        exit 1
        ;;
    esac
  done
  case "$mode" in
    check-index)
      run_check_index
      ;;
    *)
      run_regen
      ;;
  esac
fi
