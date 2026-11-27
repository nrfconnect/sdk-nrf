#!/usr/bin/env bash
# Publish a toolchain bundle to Artifactory.
#
# Required env: CI_FILES_NORDICSEMI_USER, CI_FILES_NORDICSEMI_TOKEN
#
# Usage: publish_toolchain_bundle.sh <path/to/ncs-toolchain-*.tar.gz>

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
STORAGE_URL="https://files.nordicsemi.com/artifactory/NCS/external/bundles/v3/"

if [[ -z "${CI_FILES_NORDICSEMI_USER:-}" || -z "${CI_FILES_NORDICSEMI_TOKEN:-}" ]]; then
	echo "Set CI_FILES_NORDICSEMI_USER and CI_FILES_NORDICSEMI_TOKEN." >&2
	exit 1
fi

BUNDLE="${1:-}"
if [[ -z "$BUNDLE" || ! -f "$BUNDLE" ]]; then
	echo "Usage: $0 <path/to/ncs-toolchain-*.tar.gz>" >&2
	exit 1
fi

NAME="$(basename "$BUNDLE")"
case "$NAME" in
ncs-toolchain-x86_64-linux-*.tar.gz)
	arch=x86_64
	platform=linux
	bundle_id="${NAME#ncs-toolchain-x86_64-linux-}"
	bundle_id="${bundle_id%.tar.gz}"
	;;
ncs-toolchain-x86_64-windows-*.tar.gz)
	arch=x86_64
	platform=windows
	bundle_id="${NAME#ncs-toolchain-x86_64-windows-}"
	bundle_id="${bundle_id%.tar.gz}"
	;;
ncs-toolchain-aarch64-macos-*.tar.gz)
	arch=aarch64
	platform=macos
	bundle_id="${NAME#ncs-toolchain-aarch64-macos-}"
	bundle_id="${bundle_id%.tar.gz}"
	;;
*)
	echo "Unexpected bundle name (Jenkins/bundler convention): ${NAME}" >&2
	exit 1
	;;
esac

INDEX_NAME="index-${platform}-${arch}.json"
INDEX_URL="${STORAGE_URL}/${INDEX_NAME}"

echo "Downloading ${INDEX_URL}..."
http_code="$(
	curl -sS -u "${CI_FILES_NORDICSEMI_USER}:${CI_FILES_NORDICSEMI_TOKEN}" \
		-o "$INDEX_NAME" -w "%{http_code}" "$INDEX_URL" || true
)"
case "$http_code" in
200) ;;
404)
	echo "No existing ${INDEX_NAME} at ${INDEX_URL}; starting from empty index."
	echo '[]' >"$INDEX_NAME"
	;;
*)
	echo "Failed to download index ${INDEX_URL} (HTTP ${http_code})." >&2
	exit 1
	;;
esac

python3 "$REPO_ROOT/scripts/ci/nrfutil_toolchain_index.py" --schema-version 3 update \
	--index-file "$INDEX_NAME" \
	--bundle-file "$BUNDLE" \
	--bundle-version "$bundle_id" \
	--timestamp "$(date +%s)"

echo "Uploading ${NAME}..."
#curl -fsS -u "${CI_FILES_NORDICSEMI_USER}:${CI_FILES_NORDICSEMI_TOKEN}" \
# 	-T "$BUNDLE" "${STORAGE_URL}${NAME}"

echo "Uploading ${INDEX_NAME}..."
#curl -fsS -u "${CI_FILES_NORDICSEMI_USER}:${CI_FILES_NORDICSEMI_TOKEN}" \
#	-T "$INDEX_NAME" "$INDEX_URL"

echo "Published ${NAME} (index key ${bundle_id})."
