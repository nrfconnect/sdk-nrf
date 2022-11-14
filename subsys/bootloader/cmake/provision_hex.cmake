#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This CMakeLists.txt is executed only by the parent application
# and generates the provision.hex file.

# Build and include hex file containing provisioned data for the bootloader.
set(NRF_SCRIPTS            ${NRF_DIR}/scripts)
set(NRF_BOOTLOADER_SCRIPTS ${NRF_SCRIPTS}/bootloader)
set(PROVISION_HEX_NAME     provision.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})

if(CONFIG_SECURE_BOOT)
    if (DEFINED CONFIG_SB_MONOTONIC_COUNTER)
      set(monotonic_counter_arg
        --num-counter-slots-version ${CONFIG_SB_NUM_VER_COUNTER_SLOTS})
    endif()


# Skip signing if MCUBoot is to be booted and its not built from source
    if ((CONFIG_SB_VALIDATE_FW_SIGNATURE OR CONFIG_SB_VALIDATE_FW_HASH) AND
        NOT (CONFIG_BOOTLOADER_MCUBOOT AND NOT CONFIG_MCUBOOT_BUILD_STRATEGY_FROM_SOURCE))

      include(${CMAKE_CURRENT_LIST_DIR}/debug_keys.cmake)
      include(${CMAKE_CURRENT_LIST_DIR}/sign.cmake)

      if (${CONFIG_SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST})
        set(public_keys_file_arg
          --public-key-files "${PUBLIC_KEY_FILES},${SIGNATURE_PUBLIC_KEY_FILE}"
          )
        message(WARNING
          "
          -----------------------------------------------------------------
          --- WARNING: SB_DEBUG_SIGNATURE_PUBLIC_KEY_LAST is enabled.   ---
          --- This config should only be enabled for testing/debugging. ---
          -----------------------------------------------------------------")
        else()
          set(public_keys_file_arg
            --public-key-files "${SIGNATURE_PUBLIC_KEY_FILE},${PUBLIC_KEY_FILES}"
            )
        endif()
      set(PROVISION_DEPENDS signature_public_key_file_target)
    endif()

    if (CONFIG_SOC_NRF5340_CPUNET)
      set(s0_arg --s0-addr $<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>)
    else()
      set(s0_arg --s0-addr $<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>)
      set(s1_arg --s1-addr $<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>)
    endif()

    if (CONFIG_SB_DEBUG_NO_VERIFY_HASHES)
      set(no_verify_hashes_arg --no-verify-hashes)
    endif()
endif()

if(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
  get_shared(mcuboot_hw_counters_num IMAGE mcuboot PROPERTY MCUBOOT_HW_COUNTERS_NUM)

  # When Nordic Secure Immutable Bootloader is enabled MCUBOOT itself can be updated.
  # Thus one of the updateable images for MCUBOOT will be  MUCBOOT itself.
  # We don't want to use a hardware security counter for the MCUBOOT image inside
  # MCUBOOT becuase the NSIB will use a hardware security counter for MCUBOOT anyway.
  if (CONFIG_SECURE_BOOT)
    math(EXPR mcuboot_hw_counters_num "${mcuboot_hw_counters_num} - 1")
  endif()
  set(mcuboot_counters_num --mcuboot-counters-num ${mcuboot_hw_counters_num})
  set(mcuboot_counters_slots --mcuboot-counters-slots ${CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS})
else()
  set(mcuboot_counters_num --mcuboot-counters-num "0")
  set(mcuboot_counters_slots --mcuboot-counters-slots "0")
endif()

if(CONFIG_SECURE_BOOT)
add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/provision.py
  ${s0_arg}
  ${s1_arg}
  --provision-addr $<TARGET_PROPERTY:partition_manager,PM_PROVISION_ADDRESS>
  ${public_keys_file_arg}
  --output ${PROVISION_HEX}
  ${monotonic_counter_arg}
  --max-size ${CONFIG_PM_PARTITION_SIZE_PROVISION}
  ${no_verify_hashes_arg}
  ${mcuboot_counters_num}
  ${mcuboot_counters_slots}
  DEPENDS
  ${PROVISION_KEY_DEPENDS}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating data to be provisioned to the Bootloader, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
  )
elseif(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/provision.py
  --mcuboot-only
  --provision-addr $<TARGET_PROPERTY:partition_manager,PM_PROVISION_ADDRESS>
  --output ${PROVISION_HEX}
  --max-size ${CONFIG_PM_PARTITION_SIZE_PROVISION}
  ${mcuboot_counters_num}
  ${mcuboot_counters_slots}
  DEPENDS
  ${PROVISION_KEY_DEPENDS}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating data to be provisioned to the Bootloader, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
  )
endif()


if(CONFIG_SECURE_BOOT OR CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
  add_custom_target(
    provision_target
    DEPENDS
    ${PROVISION_HEX}
    ${PROVISION_DEPENDS}
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

endif()
