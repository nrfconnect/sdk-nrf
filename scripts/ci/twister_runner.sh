#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(dirname "$0")
TOOLCHAIN_BUNDLE_ID=$("$SCRIPT_DIR/../print_toolchain_checksum.sh")

EXTRA_FLAGS=""
NRFUTIL=""
OS="$(uname)"

echo "Detected OS: $OS"
echo "Toolchain bundle: $TOOLCHAIN_BUNDLE_ID"
echo "Using twister arguments: $@"

# Detect OS and set OS-specific commands
case "$(uname -s)" in
    MINGW*)
        EXTRA_FLAGS="--short-build-path"
        export PYTHONUTF8=1
        NRFUTIL="../nrfutil.exe sdk-manager toolchain launch \
--toolchain-bundle-id $TOOLCHAIN_BUNDLE_ID -- python zephyr\\scripts\\twister"
        ;;
    Linux*)
        NRFUTIL="../nrfutil sdk-manager toolchain launch \
--toolchain-bundle-id $TOOLCHAIN_BUNDLE_ID -- python zephyr/scripts/twister"
        ;;
    Darwin*)
        EXTRA_FLAGS=""
        NRFUTIL="sudo ../nrfutil sdk-manager toolchain launch \
--toolchain-bundle-id $TOOLCHAIN_BUNDLE_ID -- python zephyr/scripts/twister"
        ;;
    *)
        echo "Unsupported OS: $(uname -s)"
        exit 1
        ;;
esac

set -x

$NRFUTIL "$@" $EXTRA_FLAGS
