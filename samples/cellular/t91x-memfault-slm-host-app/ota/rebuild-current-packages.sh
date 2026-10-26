#!/usr/bin/env bash
set -euo pipefail

OTA_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(cd -- "$OTA_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$APP_DIR/build}"

NRF91_APP_IMAGE="${NRF91_APP_IMAGE:-/Users/max/work/repos/slm-workspace/ncs-serial-modem/app/twister-out/nrf9151dk_nrf9151_ns/zephyr_gnu/ncs-serial-modem/app/serial_modem.t91x_alternate/app/zephyr/zephyr.signed.bin}"
NRF91_MODEM_FULL_IMAGE="${NRF91_MODEM_FULL_IMAGE:-/Users/max/work/nrfcloud-canonical-symbol-files/pc-nrfconnect-quickstart/resources/devices/firmware/mfw_nrf91x1_2.0.4/mfw_nrf91x1_2.0.4.cbor}"

APP_IMAGE="$BUILD_DIR/t91x-memfault-slm-host-app/zephyr/zephyr.signed.bin"
NET_IMAGE="$BUILD_DIR/signed_by_mcuboot_and_b0_hci_ipc.bin"
BL_S0_IMAGE="$BUILD_DIR/signed_by_mcuboot_and_b0_mcuboot.bin"
BL_S1_IMAGE="$BUILD_DIR/signed_by_mcuboot_and_b0_mcuboot_s1_variant.bin"
APP_CONFIG="$BUILD_DIR/t91x-memfault-slm-host-app/zephyr/.config"
NET_CONFIG="$BUILD_DIR/hci_ipc/zephyr/.config"
APP_VERSION_HEADER="$BUILD_DIR/t91x-memfault-slm-host-app/zephyr/include/generated/zephyr/version.h"
NET_VERSION_HEADER="$BUILD_DIR/hci_ipc/zephyr/include/generated/zephyr/version.h"

require_file() {
	if [[ ! -f "$1" ]]; then
		printf 'Required input not found: %s\n' "$1" >&2
		exit 1
	fi
}

read_config() {
	sed -n "s/^$2=//p" "$1" | head -n 1
}

read_build_version() {
	sed -n 's/^#define BUILD_VERSION //p' "$1" | head -n 1
}

for input in \
	"$APP_IMAGE" \
	"$NET_IMAGE" \
	"$BL_S0_IMAGE" \
	"$BL_S1_IMAGE" \
	"$APP_CONFIG" \
	"$NET_CONFIG" \
	"$APP_VERSION_HEADER" \
	"$NET_VERSION_HEADER" \
	"$NRF91_APP_IMAGE" \
	"$NRF91_MODEM_FULL_IMAGE"; do
	require_file "$input"
done

python3 -c 'import cbor2' >/dev/null

python3 "$OTA_DIR/ota-bundle.py" \
	-o "$OTA_DIR/t91x-current-app-net-modem.bin" \
	nrf53_app="$APP_IMAGE" \
	nrf53_net="$NET_IMAGE" \
	nrf91_app="$NRF91_APP_IMAGE" \
	nrf91_mdm_full="$NRF91_MODEM_FULL_IMAGE"

python3 "$OTA_DIR/ota-bundle.py" \
	-o "$OTA_DIR/t91x-current-nrf53-bl-s0.bin" \
	nrf53_bl="$BL_S0_IMAGE"

python3 "$OTA_DIR/ota-bundle.py" \
	-o "$OTA_DIR/t91x-current-nrf53-bl-s1.bin" \
	nrf53_bl="$BL_S1_IMAGE"

APP_FW_INFO_VERSION="$(read_config "$APP_CONFIG" CONFIG_FW_INFO_FIRMWARE_VERSION)"
NET_FW_INFO_VERSION="$(read_config "$NET_CONFIG" CONFIG_FW_INFO_FIRMWARE_VERSION)"
APP_BUILD_VERSION="$(read_build_version "$APP_VERSION_HEADER")"
NET_BUILD_VERSION="$(read_build_version "$NET_VERSION_HEADER")"

for value in \
	"$APP_FW_INFO_VERSION" \
	"$NET_FW_INFO_VERSION" \
	"$APP_BUILD_VERSION" \
	"$NET_BUILD_VERSION"; do
	if [[ -z "$value" ]]; then
		printf 'Could not read firmware version metadata from the current build.\n' >&2
		exit 1
	fi
done

{
	printf 'Bundle: t91x-current-app-net-modem.bin\n\n'
	printf 'nrf53_app:\n  fw_info_version: %s\n  build: %s\n\n' \
		"$APP_FW_INFO_VERSION" "$APP_BUILD_VERSION"
	printf 'nrf53_net:\n  fw_info_version: %s\n  build: %s\n\n' \
		"$NET_FW_INFO_VERSION" "$NET_BUILD_VERSION"
	printf 'nrf91_app:\n  source: %s\n\n' "$(basename -- "$NRF91_APP_IMAGE")"
	printf 'nrf91_mdm_full:\n  source: %s\n\n' "$(basename -- "$NRF91_MODEM_FULL_IMAGE")"
	printf 'Note: ota_bundle.cddl records element target and size, but does not encode\n'
	printf 'per-image version metadata. fw_info_version is embedded in the signed image.\n'
} > "$OTA_DIR/t91x-current-app-net-modem.versions.txt"

for bundle in \
	"$OTA_DIR/t91x-current-app-net-modem.bin" \
	"$OTA_DIR/t91x-current-nrf53-bl-s0.bin" \
	"$OTA_DIR/t91x-current-nrf53-bl-s1.bin"; do
	python3 "$OTA_DIR/ota-bundle-check.py" "$bundle"
done