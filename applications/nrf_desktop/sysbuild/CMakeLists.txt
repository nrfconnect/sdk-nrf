#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# The application uses the configuration/<board> scheme for configuration files.
set(SB_APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/../configuration/\${NORMALIZED_BOARD_TARGET}")

# The main application configuration directory is defined by the main application's
# `CMakeLists.txt` file (this allows to avoid dependency on main application name).
# Redirect other images' configuration directories to redefine configuration.
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
set(uicr_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../configuration/\${NORMALIZED_BOARD_TARGET}/images/uicr"
    CACHE INTERNAL "Application configuration dir controlled by sysbuild"
)

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)

if(SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU)
  message(WARNING "
          ------------------------------------------------------------------------------
          --- WARNING: MCUboot signature verification uses KMU-stored keys. You must ---
          --- use the `west flash` command with either the `--erase` or `--recover`  ---
          --- option to ensure the bootloader provisioning operation is included in  ---
          --- the flashing procedure. The application will fail to boot if MCUboot   ---
          --- is not properly provisioned. For more details, see the `Building and   ---
          --- running` section from `Application description` page in nRF Desktop    ---
          --- application documentation.                                             ---
          ------------------------------------------------------------------------------
          ")
endif()
