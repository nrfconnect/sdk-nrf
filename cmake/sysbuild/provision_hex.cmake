#
# Copyright (c) 2022-2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This CMakeLists.txt is executed only by sysbuild
# and generates the provision.hex file.

include(${CMAKE_CURRENT_LIST_DIR}/sign.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/debug_keys.cmake)

function(provision application prefix_name)
  ExternalProject_Get_Property(${application} BINARY_DIR)
  import_kconfig(CONFIG_ ${BINARY_DIR}/zephyr/.config)

  # Build and include hex file containing provisioned data for the bootloader.
  set(PROVISION_HEX_NAME     ${prefix_name}provision.hex)
  set(PROVISION_HEX          ${CMAKE_BINARY_DIR}/${PROVISION_HEX_NAME})

  if(CONFIG_SECURE_BOOT)
    if(DEFINED CONFIG_SB_MONOTONIC_COUNTER)
      set(monotonic_counter_arg
          --num-counter-slots-version ${CONFIG_SB_NUM_VER_COUNTER_SLOTS})
    endif()

    # Skip signing if MCUBoot is to be booted and its not built from source
    if((CONFIG_SB_VALIDATE_FW_SIGNATURE OR CONFIG_SB_VALIDATE_FW_HASH) AND
        NCS_SYSBUILD_PARTITION_MANAGER)

      if (${SB_CONFIG_SECURE_BOOT_DEBUG_SIGNATURE_PUBLIC_KEY_LAST})
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
      string(REPLACE ";" "," PUBLIC_KEY_FILES "${PUBLIC_KEY_FILES}")

      set(public_keys_file_arg
        --public-key-files "${PUBLIC_KEY_FILES}"
      )

      set(PROVISION_DEPENDS signature_public_key_file_target
          ${application}_extra_byproducts
          ${CMAKE_BINARY_DIR}/${application}/zephyr/.config
          ${SIGNATURE_PUBLIC_KEY_FILE}
          ${PROJECT_BINARY_DIR}/.config
      )
    endif()

    # Adjustment to be able to load into sysbuild
    if(CONFIG_SOC_NRF5340_CPUNET OR "${domain}" STREQUAL "CPUNET")
      set(partition_manager_target partition_manager_CPUNET)
      set(s0_arg --s0-addr $<TARGET_PROPERTY:${partition_manager_target},PM_APP_ADDRESS>)
      set(s1_arg)
    else()
      set(partition_manager_target partition_manager)
      set(s0_arg --s0-addr $<TARGET_PROPERTY:${partition_manager_target},PM_S0_ADDRESS>)
      set(s1_arg --s1-addr $<TARGET_PROPERTY:${partition_manager_target},PM_S1_ADDRESS>)
    endif()

    if(SB_CONFIG_SECURE_BOOT_DEBUG_NO_VERIFY_HASHES)
      set(no_verify_hashes_arg --no-verify-hashes)
    endif()

    b0_sign_image(${application})
    if(NOT (CONFIG_SOC_NRF5340_CPUNET OR "${domain}" STREQUAL "CPUNET") AND SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
      b0_sign_image("s1_image")
    endif()
  endif()

  if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    set(mcuboot_counters_slots --mcuboot-counters-slots ${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS})
  endif()

  if(CONFIG_SECURE_BOOT)
    add_custom_command(
      OUTPUT
      ${PROVISION_HEX}
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/provision.py
      ${s0_arg}
      ${s1_arg}
      --provision-addr $<TARGET_PROPERTY:${partition_manager_target},PM_PROVISION_ADDRESS>
      ${public_keys_file_arg}
      --output ${PROVISION_HEX}
      --max-size ${CONFIG_PM_PARTITION_SIZE_PROVISION}
      ${monotonic_counter_arg}
      ${no_verify_hashes_arg}
      ${mcuboot_counters_slots}
      DEPENDS
      ${PROVISION_KEY_DEPENDS}
      ${PROVISION_DEPENDS}
      WORKING_DIRECTORY
      ${PROJECT_BINARY_DIR}
      COMMENT
      "Creating data to be provisioned to the Bootloader, storing to ${PROVISION_HEX_NAME}"
      USES_TERMINAL
    )
  elseif(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    add_custom_command(
      OUTPUT
      ${PROVISION_HEX}
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/provision.py
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

  add_custom_target(
    ${prefix_name}provision_target
    DEPENDS
    ${PROVISION_HEX}
    ${PROVISION_DEPENDS}
    )

  get_property(
    ${prefix_name}provision_set
    GLOBAL PROPERTY ${prefix_name}provision_PM_HEX_FILE SET
    )

  if(NOT ${prefix_name}provision_set)
    # Set hex file and target for the 'provision' placeholder partition.
    # This includes the hex file (and its corresponding target) to the build.
    set_property(
      GLOBAL PROPERTY
      ${prefix_name}provision_PM_HEX_FILE
      ${PROVISION_HEX}
      )

    set_property(
      GLOBAL PROPERTY
      ${prefix_name}provision_PM_TARGET
      ${prefix_name}provision_target
      )
  endif()
endfunction()

if(NCS_SYSBUILD_PARTITION_MANAGER)
  b0_gen_keys()

  # Get the main app of the domain that secure boot should handle.
  if(SB_CONFIG_SECURE_BOOT AND SB_CONFIG_SECURE_BOOT_APPCORE)
    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      provision("mcuboot" "app_")
    else()
      provision("${DEFAULT_IMAGE}" "app_")
    endif()
  elseif(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    provision("${DEFAULT_IMAGE}" "app_")
  endif()

  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    get_property(main_app GLOBAL PROPERTY DOMAIN_APP_CPUNET)

    if(NOT main_app)
      message(FATAL_ERROR "Secure boot is enabled on domain CPUNET"
                          " but no image is selected for this domain.")
    endif()

    provision("${main_app}" "net_")
  endif()
endif()
