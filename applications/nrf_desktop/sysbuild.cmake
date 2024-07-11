#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Redirect configuration directories of sysbuild images to use those in the board
# configuration directory
set(mcuboot_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/configuration/\${NORMALIZED_BOARD_TARGET}/images/mcuboot"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)
set(b0_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/configuration/\${NORMALIZED_BOARD_TARGET}/images/b0"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)
set(ipc_radio_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/configuration/\${NORMALIZED_BOARD_TARGET}/images/ipc_radio"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)

# Application-specific sysbuild image is configured in its source directory.
set(gpio_expander_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/images/gpio_expander"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)

# Add application-specific sysbuild images.
include("${CMAKE_CURRENT_LIST_DIR}/images/sysbuild/CMakeLists.txt")
