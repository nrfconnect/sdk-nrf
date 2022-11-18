#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This CMake code adds commands for generating a provision.hex file to
# the build. It can only be executed by the parent image.
#
# The provision.hex file contains provisioned data for the bootloader.

set(PROVISION_HEX_NAME     provision.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})

if(CONFIG_SECURE_BOOT)
    if (DEFINED CONFIG_SB_MONOTONIC_COUNTER)
      set(monotonic_counter_arg
        --num-counter-slots-version ${CONFIG_SB_NUM_VER_COUNTER_SLOTS})
    endif()

	# Skip signing if MCUBoot is to be booted and it is not built from source
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
  set(mcuboot_counters_slots --mcuboot-counters-slots ${CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS})

  if (NOT CONFIG_SECURE_BOOT)
	set(mcuboot_only --mcuboot-only)
  endif()
endif()

if(CONFIG_SECURE_BOOT)
  set(pm_provision_bl_storage_address
	--pm-provision-bl-storage-address
	$<TARGET_PROPERTY:partition_manager,PM_PROVISION_BL_STORAGE_ADDRESS>
  )

  set(pm_provision_bl_storage_size
	--pm-provision-bl-storage-size
	$<TARGET_PROPERTY:partition_manager,PM_PROVISION_BL_STORAGE_SIZE>
  )
endif()

if(CONFIG_SECURE_BOOT OR CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
  add_custom_command(
	OUTPUT
	${PROVISION_HEX}
	COMMAND
	${PYTHON_EXECUTABLE}
	${NRF_DIR}/scripts/bootloader/provision.py
	${s0_arg}
	${s1_arg}
	${mcuboot_only}
	--pm-provision-counters-address $<TARGET_PROPERTY:partition_manager,PM_PROVISION_COUNTERS_ADDRESS>
	--pm-provision-counters-size    $<TARGET_PROPERTY:partition_manager,PM_PROVISION_COUNTERS_SIZE>
	${pm_provision_bl_storage_address}
	${pm_provision_bl_storage_size}
	${public_keys_file_arg}
	--output ${PROVISION_HEX}
	${monotonic_counter_arg}
	${no_verify_hashes_arg}
	${mcuboot_counters_slots}
	DEPENDS
	${PROVISION_KEY_DEPENDS}
	WORKING_DIRECTORY
	${PROJECT_BINARY_DIR}
	COMMENT
	"Creating data to be provisioned to the Bootloader, storing to ${PROVISION_HEX_NAME}"
	USES_TERMINAL
  )

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
