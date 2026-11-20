#!/usr/bin/env bash
# Build an NCS toolchain bundle (Linux / Windows / macOS).
#
# Required env (Artifactory / internal nrfutil packages):
#   CI_FILES_NORDICSEMI_USER
#   CI_FILES_NORDICSEMI_TOKEN
#
# Optional: MANIFEST_LABEL — nrfutil manifest ref (default: tag name on tag build, else HEAD SHA).
#
# Linux host must provide: git, curl, debootstrap, fakechroot, fakeroot
# (installed by .github/workflows/create-toolchain-bundle.yml in CI).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$REPO_ROOT"
# Git runs before the Linux branch (manifest ref); containers need this (see Jenkinsfile.bundle).
git config --global --add safe.directory "$REPO_ROOT"

# Tag push / dispatch on tag: uses TAG_NAME. Anything else (PRs): use commit SHA.
if [[ -z "${MANIFEST_LABEL:-}" ]]; then
	if [[ "${GITHUB_REF_TYPE:-}" == tag ]]; then
		MANIFEST_LABEL=$GITHUB_REF_NAME
	else
		MANIFEST_LABEL=$(git rev-parse HEAD)
	fi
fi
if ! git rev-parse --verify "${MANIFEST_LABEL}^{commit}" >/dev/null 2>&1; then
	git checkout -B "$MANIFEST_LABEL"
fi
NRFUTIL_BASE="https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables"
NRFUTIL_INTERNAL_INDEX="https://files.nordicsemi.com/artifactory/swtools/internal/nrfutil/index/init.json"

if [[ -z "${CI_FILES_NORDICSEMI_USER:-}" || -z "${CI_FILES_NORDICSEMI_TOKEN:-}" ]]; then
	echo "Set CI_FILES_NORDICSEMI_USER and CI_FILES_NORDICSEMI_TOKEN." >&2
	exit 1
fi

toolchain_cmake_version() {
	local bundle_id=$1 major minor
	if grep -qE '^\s*VERSION_MAJOR\s*=' VERSION; then
		major=$(grep -E '^\s*VERSION_MAJOR\s*=' VERSION | sed -E 's/.*=\s*([0-9]+).*/\1/')
		minor=$(grep -E '^\s*VERSION_MINOR\s*=' VERSION | sed -E 's/.*=\s*([0-9]+).*/\1/')
	else
		local head
		head=$(grep -oE '^[0-9]+\.[0-9]+' VERSION | head -1)
		major=${head%%.*}
		minor=${head#*.}
	fi
	printf '%s.%s.%s.%d\n' "$major" "$minor" "$(date -u +%Y%m%d)" "$((16#${bundle_id}))"
}

install_nrfutil_bundler() {
	curl -fsSL "$NRFUTIL_URL" -o "$NRFUTIL"
	chmod +x "$NRFUTIL"
	"$NRFUTIL" self-upgrade
	"$NRFUTIL" install sdk-manager
	if "$NRFUTIL" sdk-manager toolchain search --category bundle-id | grep -qF "$BUNDLE_ID"; then
		echo "Toolchain bundle ${BUNDLE_ID} already in index; nothing to build."
		if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
			echo "bundle_id=$BUNDLE_ID" >> "$GITHUB_OUTPUT"
		fi
		exit 0
	fi
	"$NRFUTIL" config package-index add internal-production "$NRFUTIL_INTERNAL_INDEX" \
		--environment-variable-name CI_FILES_NORDICSEMI_TOKEN
	"$NRFUTIL" install --force --package-index-name internal-production toolchain-bundler=0.26.0
}

run_bundler_build() {
	"$NRFUTIL" toolchain-bundler build \
		--cmake-toolchain-version "$CMAKE_TOOLCHAIN_VERSION" \
		--output-dir output \
		--ncs-repository "$REPO_URI" \
		"$@" \
		"$MANIFEST_LABEL"
}

BUNDLE_ID=$("$REPO_ROOT/scripts/print_toolchain_checksum.sh")
CMAKE_TOOLCHAIN_VERSION=$(toolchain_cmake_version "$BUNDLE_ID")
OS="$(uname -s)"

mkdir -p output

# Per-OS setup, nrfutil install, and build toolchain bundle.
case "$OS" in
Linux*)
	NRFUTIL="$REPO_ROOT/nrfutil"
	NRFUTIL_URL="${NRFUTIL_BASE}/x86_64-unknown-linux-gnu/nrfutil"
	REPO_URI="file://${REPO_ROOT//\\//}"
	install_nrfutil_bundler
	run_bundler_build --keep-env-vars CI_FILES_NORDICSEMI_TOKEN
	;;
MINGW* | MSYS* | CYGWIN*)
	git config --global core.autocrlf false
	NRFUTIL="$REPO_ROOT/nrfutil.exe"
	NRFUTIL_URL="${NRFUTIL_BASE}/x86_64-pc-windows-msvc/nrfutil.exe"
	REPO_URI="file:///$(cygpath -m "$REPO_ROOT")"
	install_nrfutil_bundler
	run_bundler_build --allow-changes-to-host
	;;
Darwin*)
	export PATH="/opt/homebrew/bin:/opt/homebrew/opt/coreutils/libexec/gnubin:$PATH"
	NRFUTIL="$REPO_ROOT/nrfutil"
	NRFUTIL_URL="${NRFUTIL_BASE}/universal-apple-darwin/nrfutil"
	REPO_URI="file://${REPO_ROOT//\\//}"
	if [[ ! -w /opt/nordic/ncs ]]; then
		sudo mkdir -p /opt/nordic/ncs
		sudo chown -R "$(whoami)" /opt/nordic/ncs
	fi
	install_nrfutil_bundler
	run_bundler_build
	;;
*)
	echo "Unsupported OS: $OS" >&2
	exit 1
	;;
esac

if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
	echo "bundle_id=$BUNDLE_ID" >> "$GITHUB_OUTPUT"
fi
