#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# CONFIG_SECURE_BOOT can only be set for root app
if (FIRST_BOILERPLATE_EXECUTION AND CONFIG_SECURE_BOOT)
  if (CONFIG_BOOTLOADER_MCUBOOT)
    set(image_to_boot mcuboot_)
    set(s0_addr $<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>)
    set(s1_addr $<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>)
  else()
    if (CONFIG_SPM)
      set(image_to_boot spm_)
      set(s0_addr $<TARGET_PROPERTY:partition_manager,PM_SPM_ADDRESS>)
    else()
      set(app_CONFIG_BOOTED_BY_SECURE_BOOT "y" CACHE STRING "Booted by B0")
      set(s0_addr $<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>)
    endif()
  endif()

  # Include a kconfig file which enables CONFIG_FW_METADATA in the image
  # which is booted by B0
  set(old_conf ${${image_to_boot}CONF_FILE})
  set(${image_to_boot}CONF_FILE
    "${old_conf} ${CMAKE_CURRENT_SOURCE_DIR}/fw_metadata.conf"
    CACHE STRING "" FORCE)
endif()

zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/../include)
if (NOT DEFINED CONFIG_IS_SECURE_BOOTLOADER)
  add_subdirectory(bl_crypto_client)
endif()
set(NRF_BOOTLOADER_SCRIPTS ${NRF_DIR}/scripts/bootloader)
set(PROVISION_HEX_NAME     provision.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/debug_keys.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/sign.cmake)

add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/provision.py
  --s0-addr ${s0_addr}
  --s1-addr ${s1_addr}
  --provision-addr $<TARGET_PROPERTY:partition_manager,PM_PROVISION_ADDRESS>
  --public-key-files "${SIGNATURE_PUBLIC_KEY_FILE},${PUBLIC_KEY_FILES}"
  --output ${PROVISION_HEX}
  DEPENDS
  ${PROVISION_KEY_DEPENDS}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating provision data for Bootloader, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
  )

add_custom_target(
  provision_target
  DEPENDS
  ${PROVISION_HEX}
  signature_public_key_file_target
  )

# Set hex file and target for the 'provision' placeholder partition.
# This includes the hex file (and its corresponding target) to the build.
set_property(
  GLOBAL PROPERTY
  provision_PM_HEX_FILE
  ${PROVISION_HEX}
  )

set_property(
  GLOBAL PROPERTY
  provision_PM_TARGET
  provision_target
  )
