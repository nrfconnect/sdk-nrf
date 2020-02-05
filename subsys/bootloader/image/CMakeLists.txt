#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

function(add_overlay_config image overlay_file)
  set(old_conf ${${image}OVERLAY_CONFIG})
  string(FIND "${old_conf}" "${overlay_file}" found)
  if (${found} EQUAL -1)
    set(${image}OVERLAY_CONFIG
      "${old_conf} ${overlay_file}"
      CACHE STRING "" FORCE)
  endif()
endfunction()

# This CMakeLists.txt is executed by the root application only when
# B0 (SECURE_BOOT) is enabled. First, figure out what image will be
# booted by B0, and set the required properties for that image to be
# bootable by B0.

if (CONFIG_BOOTLOADER_MCUBOOT)
  set(image_to_boot mcuboot_)

  add_overlay_config(
    mcuboot_
    ${CMAKE_CURRENT_SOURCE_DIR}/multi_image_mcuboot.conf
    )

  # Partition S1 is only enabled when MCUBoot is enabled.
  set(s1_addr $<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>)

else()

  if (CONFIG_SPM)
    set(image_to_boot spm_)
  endif()
endif()

if (image_to_boot)
  # Include a kconfig file which enables CONFIG_FW_INFO in the image
  # which is booted by B0.
  add_overlay_config(
    ${image_to_boot}
    ${CMAKE_CURRENT_SOURCE_DIR}/fw_info.conf
    )
else()
  assert(CONFIG_FW_INFO "CONFIG_FW_INFO must be set")
endif()

# Partition S0 will always contain the image to be booted by B0.
set(s0_addr $<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>)
if (s1_addr)
  set(s1_arg --s1-addr ${s1_addr})
endif()

if (CONFIG_SOC_NRF9160)
  # Override PM_PROVISION_ADDRESS since Partition Manager cannot yet place
  # partitions in OTP.
  set(otp_start_addr "0xff8108")
  set(provision_addr ${otp_start_addr})
else ()
  set(provision_addr "$<TARGET_PROPERTY:partition_manager,PM_PROVISION_ADDRESS>")
endif()

# Build and include hex file with provision data about the image to be booted.
set(NRF_SCRIPTS            ${NRF_DIR}/scripts)
set(NRF_BOOTLOADER_SCRIPTS ${NRF_SCRIPTS}/bootloader)
set(PROVISION_HEX_NAME     provision.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/debug_keys.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/sign.cmake)

if (${CONFIG_SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST})
  set(ALL_PUBLIC_KEY_FILES "${PUBLIC_KEY_FILES},${SIGNATURE_PUBLIC_KEY_FILE}")
  message(WARNING
    "
      -----------------------------------------------------------------
      --- WARNING: SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST is enabled.   ---
      --- This config should only be enabled for testing/debugging. ---
      -----------------------------------------------------------------")
else()
  set(ALL_PUBLIC_KEY_FILES "${SIGNATURE_PUBLIC_KEY_FILE},${PUBLIC_KEY_FILES}")
endif()

add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/provision.py
  --s0-addr ${s0_addr}
  ${s1_arg}
  --provision-addr ${provision_addr}
  --public-key-files ${ALL_PUBLIC_KEY_FILES}
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

get_property(
  provision_set
  GLOBAL PROPERTY provision_PM_HEX_FILE SET
  )

if(NOT provision_set)
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
endif()
