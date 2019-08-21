#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# This CMakeLists.txt is executed by the root application only when
# B0 (SECURE_BOOT) is enabled. First, figure out what image will be
# booted by B0, and set the required properties for that image to be
# bootable by B0.

if (CONFIG_BOOTLOADER_MCUBOOT)
  set(image_to_boot mcuboot_)
  # Partition S1 is only enabled when MCUBoot is enabled.
  set(s1_addr $<TARGET_PROPERTY:partition_manager,PM_S1_MCUBOOT_ADDRESS>)
elseif(CONFIG_SPM)
    set(image_to_boot spm_)
endif()

# Include a kconfig file which enables CONFIG_FW_METADATA in the image
# which is booted by B0.
set(old_conf ${${image_to_boot}OVERLAY_CONFIG})
string(FIND "${old_conf}" "${CMAKE_CURRENT_SOURCE_DIR}/fw_metadata.conf" found)
if (${found} EQUAL -1)
  set(${image_to_boot}OVERLAY_CONFIG
    "${old_conf} ${CMAKE_CURRENT_SOURCE_DIR}/fw_metadata.conf"
    CACHE STRING "" FORCE)
endif()

# Partition S0 will always contain the image to be booted by B0.
set(s0_addr $<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>)
if (s1_addr)
  set(s1_arg --s1-addr ${s1_addr})
endif()

# Build and include hex file with provision data about the image to be booted.
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
  ${s1_arg}
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
