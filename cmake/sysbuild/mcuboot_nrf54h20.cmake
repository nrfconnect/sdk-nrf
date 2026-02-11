#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/sign_nrf54h20.cmake)

if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
  set(MERGED_IMAGES_HEX "mcuboot_merged.hex")
  UpdateableImage_Get(images GROUP "DEFAULT")

  check_merged_slot_boundaries("slot0_partition" "${images}")
  merge_images_nrf54h20(${MERGED_IMAGES_HEX} extra_imgtool_args "merged" "slot0_partition" "${images}")
  # Since all bootloader-enabled images are merged, disable programming subimages.
  list(REMOVE_ITEM images "${DEFAULT_IMAGE}")
  disable_programming_nrf54h20("${images}")
  mcuboot_sign_merged_nrf54h20(${MERGED_IMAGES_HEX} ${DEFAULT_IMAGE} "${extra_imgtool_args}")

  set(MERGED_IMAGES_SECONDARY_HEX "mcuboot_merged_secondary_app.hex")
  UpdateableImage_Get(variants GROUP "VARIANT")

  if(variants)
    check_merged_slot_boundaries("slot1_partition" "${variants}")
    merge_images_nrf54h20(${MERGED_IMAGES_SECONDARY_HEX}
      extra_imgtool_args "merged_secondary_app" "slot1_partition" "${variants}")
    list(REMOVE_ITEM variants "mcuboot_secondary_app")
    disable_programming_nrf54h20("${variants}")
    mcuboot_sign_merged_nrf54h20(${MERGED_IMAGES_SECONDARY_HEX}
      "mcuboot_secondary_app" "${extra_imgtool_args}")
  endif()

  set(MERGED_IMAGES_FIRMWARE_UPDATER_HEX "mcuboot_merged_firmware_updater.hex")
  FirmwareUpdaterImage_Get(firmware_updater)

  if(firmware_updater)
    check_merged_slot_boundaries("slot1_partition" "${firmware_updater}")
    merge_images_nrf54h20(${MERGED_IMAGES_FIRMWARE_UPDATER_HEX}
      extra_imgtool_args "merged_firmware_updater" "slot1_partition" "${firmware_updater}")
    list(REMOVE_ITEM firmware_updater "${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME}")
    disable_programming_nrf54h20("${firmware_updater}")
    mcuboot_sign_merged_nrf54h20(${MERGED_IMAGES_FIRMWARE_UPDATER_HEX}
      "${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME}" "${extra_imgtool_args}")
  endif()
endif()
