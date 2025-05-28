#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Set the matter snippet as a base for this project configuration.
if(NOT "matter" IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
  set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} matter CACHE STRING "" FORCE)
endif()

# Set the matter-bootloader snippet as a base for bootloader configuration.
if(NOT "matter-bootloader" IN_LIST mcuboot_SNIPPET)
  set(mcuboot_SNIPPET ${mcuboot_SNIPPET} matter-bootloader CACHE STRING "" FORCE)
endif()

# Set the matter-network-core snippet for devices that support network core.
if(SB_CONFIG_NRF_DEFAULT_IPC_RADIO)
  if(NOT "matter-network-core" IN_LIST ipc_radio_SNIPPET)
    set(ipc_radio_SNIPPET ${ipc_radio_SNIPPET} matter-network-core CACHE STRING "" FORCE)
  endif()
endif()

# If the release configuration is used, enable LTO and release snippets.
if(FILE_SUFFIX STREQUAL "release")
  if(NOT "lto" IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
    set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} lto CACHE STRING "" FORCE)
  endif()
  if(NOT "matter-release" IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
    set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} matter-release CACHE STRING "" FORCE)
  endif()
endif()

# Set the matter-internal-dfu snippet for internal-dfu builds if available.
if(FILE_SUFFIX STREQUAL "internal" AND NOT "matter-internal-dfu" IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
  set(${DEFAULT_IMAGE}_SNIPPET matter-internal-dfu ${${DEFAULT_IMAGE}_SNIPPET} CACHE STRING "" FORCE)
  set(mcuboot_SNIPPET matter-internal-dfu ${mcuboot_SNIPPET} CACHE STRING "" FORCE)
endif()
