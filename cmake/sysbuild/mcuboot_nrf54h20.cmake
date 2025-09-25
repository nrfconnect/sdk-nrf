#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/sign_nrf54h20.cmake)

if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
  set(MERGED_IMAGES_HEX "mcuboot_merged.hex")
  UpdateableImage_Get(images GROUP "DEFAULT")

  check_merged_slot_boundaries("slot0_partition" "${images}")
  merge_images_nrf54h20(${MERGED_IMAGES_HEX} "${images}")
  # Since all bootloader-enabled images are merged, disable programming subimages.
  list(REMOVE_ITEM images "${DEFAULT_IMAGE}")
  disable_programming_nrf54h20("${images}")
  mcuboot_sign_merged_nrf54h20(${MERGED_IMAGES_HEX} ${DEFAULT_IMAGE})

  set(MERGED_IMAGES_SECONDARY_HEX "mcuboot_merged_secondary_app.hex")
  UpdateableImage_Get(variants GROUP "VARIANT")

  if(variants)
    check_merged_slot_boundaries("slot1_partition" "${variants}")
    merge_images_nrf54h20(${MERGED_IMAGES_SECONDARY_HEX} "${variants}")
    list(REMOVE_ITEM variants "mcuboot_secondary_app")
    disable_programming_nrf54h20("${variants}")
    mcuboot_sign_merged_nrf54h20(${MERGED_IMAGES_SECONDARY_HEX} "mcuboot_secondary_app")
  endif()
endif()
