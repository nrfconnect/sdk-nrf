# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Apply the same device tree overlay to the UICR image so both images
# see the same partition layout (periphconf after cpuapp_boot).
# Apply uicr.conf for PROTECTEDMEM and SNAPSHOT user regions.
#
# The radio-core idle image is added through the standard sysbuild network-core
# infrastructure; see Kconfig.sysbuild and sysbuild.conf (SB_CONFIG_NETCORE_REMOTE).

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

add_overlay_dts(uicr ${APP_DIR}/app.overlay)
add_overlay_config(uicr ${APP_DIR}/sysbuild/uicr.conf)
