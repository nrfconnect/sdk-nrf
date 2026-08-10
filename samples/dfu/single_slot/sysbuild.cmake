#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
  # If firmware loader images are merged, there is a necessity to use a different code-partition
  # than the slot1_partition, enforced by the FIRMWARE_LOADER_image_default.overlay, configured
  # through EXTRA_DTC_OVERLAY_FILE cache variable.
  # In the merged slot approach the slot1_partition should be defined as the merged partition and
  # code-partition should point to a subpartition of the slot1_partition.
  # The code-partition is configured in the image-specific overlay files.
  FirmwareUpdaterImage_Get(fw_loader_images)
  foreach(image IN LISTS fw_loader_images)
    # Remove the default overlay file.
    list(FILTER ${image}_EXTRA_DTC_OVERLAY_FILE EXCLUDE REGEX
      "${ZEPHYR_BASE}/share/sysbuild/image_configurations/FIRMWARE_LOADER_image_default.overlay")
    set(${image}_EXTRA_DTC_OVERLAY_FILE ${${image}_EXTRA_DTC_OVERLAY_FILE} CACHE INTERNAL
      "Application extra DTC overlay file" FORCE)
  endforeach()
endif()

# On nRF54H20 the firmware loader slot is too small for UART logging, so the board configuration
# of the firmware loader image disables it and the logging options are applied only elsewhere.
if(SB_CONFIG_FIRMWARE_LOADER_IMAGE_BLE_MCUMGR AND NOT SB_CONFIG_SOC_SERIES_NRF54H)
  include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)
  add_overlay_config(${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME}
    ${APP_DIR}/sysbuild/ble_mcumgr/log.conf
  )
endif()
