#!/bin/bash

BASEDIR=$(dirname "$0")
REQUIREMENTS=$BASEDIR/requirements-fixed.txt

# Detect OS and set tools version file + hash command
case "$(uname -s)" in
    MINGW*)   TOOLS_VERSIONS="$BASEDIR/tools-versions-win10.yml" ; HASH_CMD="sha256sum" ;;
    Linux*)   TOOLS_VERSIONS="$BASEDIR/tools-versions-linux.yml" ; HASH_CMD="sha256sum" ;;
    Darwin*)  TOOLS_VERSIONS="$BASEDIR/tools-versions-darwin.yml" ; HASH_CMD="shasum -a 256" ;;
    *) echo "Unsupported OS"; exit 1 ;;
esac

# Compute SHA256 and take first 10 characters
TOOLCHAIN_VERSION=$(cat "$REQUIREMENTS" "$TOOLS_VERSIONS" \
    | sed 's/\r$//' \
    | $HASH_CMD \
    | cut -c1-10)

echo "${TOOLCHAIN_VERSION}"
