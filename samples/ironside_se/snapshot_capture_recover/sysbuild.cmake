# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Apply the same device tree overlay to the UICR image so both images
# see the same partition layout (periphconf after cpuapp_boot).
# Apply uicr.conf for PROTECTEDMEM and SNAPSHOT user regions.

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

add_overlay_dts(uicr ${APP_DIR}/app.overlay)
add_overlay_config(uicr ${APP_DIR}/sysbuild/uicr.conf)
