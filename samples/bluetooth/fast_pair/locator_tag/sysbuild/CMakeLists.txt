#
# Copyright (c) 2024-2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# The sample uses the sysbuild/configuration/<board> scheme for the sysbuild configuration files.
set(SB_APPLICATION_CONFIG_DIR
    "${CMAKE_CURRENT_LIST_DIR}/configuration/\${NORMALIZED_BOARD_TARGET}")

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)

if(SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU)
  message(WARNING "
          ------------------------------------------------------------------------------
          --- WARNING: MCUboot signature verification uses KMU-stored keys. You must ---
          --- use the `west flash` command with either the `--erase` or `--recover`  ---
          --- option to ensure the bootloader provisioning operation is included in  ---
          --- the flashing procedure. The application will fail to boot if MCUboot   ---
          --- is not properly provisioned. For detailed instructions, refer to the   ---
          --- `Building and running` section in the Fast Pair Locator Tag Readme.    ---
          ------------------------------------------------------------------------------
          ")
endif()
