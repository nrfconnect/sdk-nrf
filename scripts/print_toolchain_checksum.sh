#!/bin/bash

BASEDIR=$(dirname "$0")
REQUIREMENTS="$BASEDIR/requirements-fixed.txt"

# OS_TYPE values:
#   windows  - Windows 10 toolchain (tools-versions-win10.yml)
#   linux  - Linux toolchain (tools-versions-linux.yml)
#   macos - macOS toolchain (tools-versions-darwin.yml)
# You can pass one of these as the first argument to override auto-detection.
# If not provided, OS_TYPE is auto-detected from uname.

# Determine OS type: argument overrides auto-detect
if [ -n "$1" ]; then
    OS_TYPE="$1"
else
    case "$(uname -s)" in
        MINGW*)  OS_TYPE="windows" ;;
        Linux*)  OS_TYPE="linux" ;;
        Darwin*) OS_TYPE="macos" ;;
        *) echo "Unsupported OS"; exit 1 ;;
    esac
fi

# Set tools-versions file and hash command based on OS_TYPE
case "$OS_TYPE" in
    windows)   TOOLS_VERSIONS="$BASEDIR/tools-versions-win10.yml" ; HASH_CMD="sha256sum" ;;
    linux)   TOOLS_VERSIONS="$BASEDIR/tools-versions-linux.yml" ; HASH_CMD="sha256sum" ;;
    macos)  TOOLS_VERSIONS="$BASEDIR/tools-versions-darwin.yml" ; HASH_CMD="shasum -a 256" ;;
    *) echo "Unsupported override argument: $OS_TYPE"; exit 1 ;;
esac

# Compute SHA256 and take first 10 characters
TOOLCHAIN_VERSION=$(cat "$REQUIREMENTS" "$TOOLS_VERSIONS" \
    | sed 's/\r$//' \
    | $HASH_CMD \
    | cut -c1-10)

echo "${TOOLCHAIN_VERSION}"
