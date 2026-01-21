# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Apply the same device tree overlay to the UICR image so both images
# see the same partition layout
# Also add secondary firmware that boots on integrity failure

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

# Add secondary firmware that boots when PROTECTEDMEM integrity check fails
ExternalZephyrProject_Add(
  APPLICATION secondary
  SOURCE_DIR ${APP_DIR}/secondary
)

# Apply app.overlay to the UICR image as well
# This ensures both the default image and UICR image see the same partition layout
add_overlay_dts(uicr ${APP_DIR}/app.overlay)
