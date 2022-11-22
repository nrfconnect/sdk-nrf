#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(PROVISION_HEX_NAME     provision_bl_storage.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})

# Skip signing if MCUBoot is to be booted and it is not built from source
if ((CONFIG_SB_VALIDATE_FW_SIGNATURE OR CONFIG_SB_VALIDATE_FW_HASH) AND
   NOT (CONFIG_BOOTLOADER_MCUBOOT AND NOT CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE))

  # Input is comma separated string, convert to CMake list type
  string(REPLACE "," ";" PUBLIC_KEY_FILES_LIST "${CONFIG_SB_PUBLIC_KEY_FILES}")

  include(${CMAKE_CURRENT_LIST_DIR}/debug_keys.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/sign.cmake)
  if (${CONFIG_SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST})
    message(WARNING
      "
      -----------------------------------------------------------------
      --- WARNING: SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST is enabled.   ---
      --- This config should only be enabled for testing/debugging. ---
      -----------------------------------------------------------------")
    list(APPEND PUBLIC_KEY_FILES ${SIGNATURE_PUBLIC_KEY_FILE})
  else()
    list(INSERT PUBLIC_KEY_FILES 0 ${SIGNATURE_PUBLIC_KEY_FILE})
  endif()

  # Convert CMake list type back to comma separated string.
  string(REPLACE ";" "," PUBLIC_KEY_FILES_COMMA_SEPARATED "${PUBLIC_KEY_FILES}")

  set(public_keys_file_arg
    --public-key-files "${PUBLIC_KEY_FILES_COMMA_SEPARATED}"
    )
  set(PROVISION_DEPENDS signature_public_key_file_target)
endif()

if (CONFIG_SOC_NRF5340_CPUNET)
  set(s0_addr PM_APP_ADDRESS)
else()
  set(s0_addr PM_S0_ADDRESS)
  set(s1_arg --s1-addr $<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>)
endif()

if (CONFIG_SB_DEBUG_NO_VERIFY_HASHES)
  set(no_verify_hashes_arg --no-verify-hashes)
endif()

add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_DIR}/scripts/bootloader/provision_bl_storage.py
  --s0-addr $<TARGET_PROPERTY:partition_manager,${s0_addr}>
  ${s1_arg}
  --address $<TARGET_PROPERTY:partition_manager,PM_PROVISION_BL_STORAGE_ADDRESS>
  --size    $<TARGET_PROPERTY:partition_manager,PM_PROVISION_BL_STORAGE_SIZE>
  --public-key-files ${public_keys_files}
  --output ${PROVISION_HEX}
  ${no_verify_hashes_arg}
  DEPENDS
  ${PROVISION_KEY_DEPENDS}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating bl_storage data to be provisioned, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
)

add_custom_target(
  provision_target_bl_storage
  DEPENDS
  ${PROVISION_HEX}
  ${PROVISION_DEPENDS}
)

get_property(
  provision_set
  GLOBAL PROPERTY provision_bl_storage_PM_HEX_FILE SET
)

if(NOT provision_set)
  # Set hex file and target for the 'provision_bl_storage'
  # placeholder partition. This includes the hex file (and its
  # corresponding target) to the build.
  set_property(
    GLOBAL PROPERTY
    provision_bl_storage_PM_HEX_FILE
    ${PROVISION_HEX}
    )

  set_property(
    GLOBAL PROPERTY
    provision_bl_storage_PM_TARGET
    provision_target_bl_storage
    )
endif()
