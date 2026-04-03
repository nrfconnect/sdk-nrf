# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_BASE}/soc/nordic/sysbuild.cmake)

# include sysbuild.cmake files from family directories if exists
include(${CMAKE_CURRENT_LIST_DIR}/${SB_CONFIG_SOC_SERIES}/sysbuild.cmake OPTIONAL)
