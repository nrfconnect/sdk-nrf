#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# The application uses the configuration/<board> scheme for configuration files.
set(SB_APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/../configuration/\${NORMALIZED_BOARD_TARGET}")

# Redirect images' configuration directories to use those in the board configuration directory
set(nrf_desktop_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../configuration/\${NORMALIZED_BOARD_TARGET}"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)
set(mcuboot_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../configuration/\${NORMALIZED_BOARD_TARGET}/images/mcuboot"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)
set(b0_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../configuration/\${NORMALIZED_BOARD_TARGET}/images/b0"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)
set(ipc_radio_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../configuration/\${NORMALIZED_BOARD_TARGET}/images/ipc_radio"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
