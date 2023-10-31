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

  if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP)
    sysbuild_get(${DEFAULT_IMAGE}_image_dir IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(${DEFAULT_IMAGE}_kernel_name IMAGE ${DEFAULT_IMAGE} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)

    list(APPEND dfu_multi_image_ids 0)
    list(APPEND dfu_multi_image_paths "${${DEFAULT_IMAGE}_image_dir}/zephyr/${${DEFAULT_IMAGE}_kernel_name}.signed.bin")
    list(APPEND dfu_multi_image_targets ${DEFAULT_IMAGE}_extra_byproducts)
  endif()

  if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET)
    list(APPEND dfu_multi_image_ids 1)
    list(APPEND dfu_multi_image_paths "${PROJECT_BINARY_DIR}/signed_by_b0_${DOMAIN_APP_CPUNET}.bin")
    list(APPEND dfu_multi_image_targets ${DOMAIN_APP_CPUNET}_extra_byproducts ${DOMAIN_APP_CPUNET}_signed_kernel_hex_target)
  endif()

  if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_MCUBOOT)
    list(APPEND dfu_multi_image_ids -2 -1)
    list(APPEND dfu_multi_image_paths "${PROJECT_BINARY_DIR}/signed_by_mcuboot_and_b0_mcuboot.bin" "${PROJECT_BINARY_DIR}/signed_by_mcuboot_and_b0_s1_image.bin")
    list(APPEND dfu_multi_image_targets mcuboot_extra_byproducts mcuboot_signed_kernel_hex_target s1_image_extra_byproducts s1_image_signed_kernel_hex_target mcuboot_signed_packaged_target s1_image_signed_packaged_target)
  endif()

  dfu_multi_image_package(dfu_multi_image_pkg
    IMAGE_IDS ${dfu_multi_image_ids}
    IMAGE_PATHS ${dfu_multi_image_paths}
    OUTPUT ${PROJECT_BINARY_DIR}/dfu_multi_image.bin
    DEPENDS ${dfu_multi_image_targets}
    )
endif()

sysbuild_get(CONFIG_ZIGBEE IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE KCONFIG)
sysbuild_get(CONFIG_ZIGBEE_FOTA IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE_FOTA KCONFIG)

if(CONFIG_ZIGBEE AND CONFIG_ZIGBEE_FOTA)
  sysbuild_get(CONFIG_ZIGBEE_FOTA_GENERATE_LEGACY_IMAGE_TYPE IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE_FOTA_GENERATE_LEGACY_IMAGE_TYPE KCONFIG)
  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION IMAGE ${DEFAULT_IMAGE} VAR CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG)
  sysbuild_get(CONFIG_ZIGBEE_FOTA_MANUFACTURER_ID IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE_FOTA_MANUFACTURER_ID KCONFIG)
  sysbuild_get(CONFIG_ZIGBEE_FOTA_IMAGE_TYPE IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE_FOTA_IMAGE_TYPE KCONFIG)
  sysbuild_get(CONFIG_ZIGBEE_FOTA_COMMENT IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE_FOTA_COMMENT KCONFIG)
  sysbuild_get(CONFIG_ZIGBEE_FOTA_MIN_HW_VERSION IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE_FOTA_MIN_HW_VERSION KCONFIG)
  sysbuild_get(CONFIG_ZIGBEE_FOTA_MAX_HW_VERSION IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ZIGBEE_FOTA_MAX_HW_VERSION KCONFIG)

  if(CONFIG_ZIGBEE_FOTA_GENERATE_LEGACY_IMAGE_TYPE)
    sysbuild_get(${DEFAULT_IMAGE}_image_dir IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(${DEFAULT_IMAGE}_kernel_name IMAGE ${DEFAULT_IMAGE} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)

    set(firmware_binary "${${DEFAULT_IMAGE}_image_dir}/zephyr/${${DEFAULT_IMAGE}_kernel_name}.signed.bin")
    set(legacy_cmd "--legacy")
  elseif(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD)
    set(firmware_binary "${PROJECT_BINARY_DIR}/dfu_multi_image.bin")
    set(legacy_cmd)
  else()
    message(FATAL_ERROR "No Zigbee FOTA image format selected. Please enable either legacy or the multi-image format.")
  endif()

  add_custom_target(zigbee_ota_image ALL
    COMMAND
    ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/zb_add_ota_header.py
      --application ${firmware_binary}
      --application-version-string ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}
      --zigbee-manufacturer-id ${CONFIG_ZIGBEE_FOTA_MANUFACTURER_ID}
      --zigbee-image-type ${CONFIG_ZIGBEE_FOTA_IMAGE_TYPE}
      --zigbee-comment ${CONFIG_ZIGBEE_FOTA_COMMENT}
      --zigbee-ota-min-hw-version ${CONFIG_ZIGBEE_FOTA_MIN_HW_VERSION}
      --zigbee-ota-max-hw-version ${CONFIG_ZIGBEE_FOTA_MAX_HW_VERSION}
      --out-directory ${PROJECT_BINARY_DIR}
      ${legacy_cmd}

    DEPENDS
    ${firmware_binary}
  )
endif(CONFIG_ZIGBEE AND CONFIG_ZIGBEE_FOTA)
