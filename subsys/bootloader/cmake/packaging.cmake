#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD)
  include(${ZEPHYR_BASE}/../nrf/cmake/fw_zip.cmake)
  include(${ZEPHYR_BASE}/../nrf/cmake/dfu_multi_image.cmake)

  set(dfu_multi_image_ids)
  set(dfu_multi_image_paths)
  set(dfu_multi_image_targets)

  sysbuild_get(dfu_multi_image_align IMAGE ${DEFAULT_IMAGE} VAR CONFIG_DFU_MULTI_IMAGE_ALIGN KCONFIG)
  if(NOT dfu_multi_image_align)
    set(dfu_multi_image_align 1)
  endif()

  if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP)
    sysbuild_get(${DEFAULT_IMAGE}_image_dir IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(${DEFAULT_IMAGE}_kernel_name IMAGE ${DEFAULT_IMAGE} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)

    list(APPEND dfu_multi_image_ids 0)

    if(SB_CONFIG_BOOT_ENCRYPTION)
      list(APPEND dfu_multi_image_paths "${${DEFAULT_IMAGE}_image_dir}/zephyr/${${DEFAULT_IMAGE}_kernel_name}.signed.encrypted.bin")
    else()
      list(APPEND dfu_multi_image_paths "${${DEFAULT_IMAGE}_image_dir}/zephyr/${${DEFAULT_IMAGE}_kernel_name}.signed.bin")
    endif()

    list(APPEND dfu_multi_image_targets ${DEFAULT_IMAGE}_extra_byproducts ${dfu_multi_image_paths})
  endif()

  if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET)
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_CPUNET)
    list(APPEND dfu_multi_image_ids 1)
    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      list(APPEND dfu_multi_image_paths "${CMAKE_BINARY_DIR}/signed_by_mcuboot_and_b0_${image_name}.bin")
      list(APPEND dfu_multi_image_targets ${image_name}_extra_byproducts ${image_name}_signed_kernel_hex_target ${image_name}_signed_packaged_target)
    else()
      list(APPEND dfu_multi_image_paths "${CMAKE_BINARY_DIR}/signed_by_b0_${image_name}.bin")
      list(APPEND dfu_multi_image_targets ${image_name}_extra_byproducts ${image_name}_signed_kernel_hex_target)
    endif()
  endif()

  if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_MCUBOOT)
    list(APPEND dfu_multi_image_ids -2 -1)
    list(APPEND dfu_multi_image_paths "${CMAKE_BINARY_DIR}/signed_by_mcuboot_and_b0_mcuboot.bin" "${CMAKE_BINARY_DIR}/signed_by_mcuboot_and_b0_s1_image.bin")
    list(APPEND dfu_multi_image_targets mcuboot_extra_byproducts mcuboot_signed_kernel_hex_target s1_image_extra_byproducts s1_image_signed_kernel_hex_target mcuboot_signed_packaged_target s1_image_signed_packaged_target)
  endif()

  if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH)
    if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET)
      list(APPEND dfu_multi_image_ids 2)
    else()
      list(APPEND dfu_multi_image_ids 1)
    endif()

    list(APPEND dfu_multi_image_paths "${CMAKE_BINARY_DIR}/nrf70.signed.bin")
    list(APPEND dfu_multi_image_targets nrf70_wifi_fw_patch_target)
  endif()

  if(DEFINED dfu_multi_image_targets)
    dfu_multi_image_package(dfu_multi_image_pkg
      IMAGE_IDS ${dfu_multi_image_ids}
      IMAGE_PATHS ${dfu_multi_image_paths}
      ALIGN ${dfu_multi_image_align}
      OUTPUT ${CMAKE_BINARY_DIR}/dfu_multi_image.bin
      DEPENDS ${dfu_multi_image_targets}
      )
  else()
    message(WARNING "No images enabled for multi image build, multi image output will not be created.")
  endif()
endif()
