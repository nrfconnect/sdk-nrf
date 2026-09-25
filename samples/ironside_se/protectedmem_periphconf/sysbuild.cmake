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

# Ensure that UICR is programmed last
sysbuild_add_dependencies(FLASH uicr ${DEFAULT_IMAGE} secondary)
