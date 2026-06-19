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

  if(CONFIG_SOC_SERIES_NRF54L)
    set(otp_write_width 4) # OTP writes are in words (4 bytes)
  else()
    set(otp_write_width 2) # OTP writes are in half-words (2 bytes)
  endif()

  if(CONFIG_SECURE_BOOT)
    if(SB_CONFIG_SECURE_BOOT_MONOTONIC_COUNTER)
      set(monotonic_counter_arg
          --num-counter-slots-version ${SB_CONFIG_SECURE_BOOT_NUM_VER_COUNTER_SLOTS})
    endif()

    # Skip signing if MCUBoot is to be booted and its not built from source
    if(CONFIG_SB_VALIDATE_FW_SIGNATURE OR CONFIG_SB_VALIDATE_FW_HASH)
      if(${SB_CONFIG_SECURE_BOOT_DEBUG_SIGNATURE_PUBLIC_KEY_LAST})
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
      set(cpunet_target y)
    else()
      set(cpunet_target n)
    endif()

    if(SB_CONFIG_SECURE_BOOT_DEBUG_NO_VERIFY_HASHES)
      set(no_verify_hashes_arg --no-verify-hashes)
    endif()

    b0_sign_image(${application} ${cpunet_target})
    if(NOT cpunet_target AND SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        b0_sign_image(mcuboot_s1_variant n)
      else()
        b0_sign_image(${DEFAULT_IMAGE}_s1_variant n)
      endif()
    endif()
  endif()

  if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    set(mcuboot_counters_slots --mcuboot-counters-slots ${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS})
  endif()

  if(SB_CONFIG_TFM_OTP_PSA_CERTIFICATE_REFERENCE AND SB_CONFIG_TFM_PSA_CERTIFICATE_REFERENCE_VALUE)
    set(psa_certificate_reference --psa-certificate-reference ${SB_CONFIG_TFM_PSA_CERTIFICATE_REFERENCE_VALUE})
  endif()

  set(provision_address)
  set(provision_size)

  if(cpunet_target)
    dt_partition_addr(s0_slot_address LABEL "s0_partition" TARGET b0n ABSOLUTE REQUIRED)
    set(s0_arg --s0-addr ${s0_slot_address})
    set(s1_arg)
  else()
    if(CONFIG_SECURE_BOOT)
      if(cpunet_target)
        dt_partition_addr(s0_slot_address LABEL "s0_partition" TARGET b0n ABSOLUTE REQUIRED)
        set(s0_arg --s0-addr ${s0_slot_address})
        set(s1_arg)
        # B0 is secure bootloader and we pick the address from its configuration.
        set(bl_storage_target b0n)
      else()
        # We can pick all of these from MCUboot image, as DTS partitions come from common
        # DTS and image header size is the same for all images for a given platform.
        dt_partition_addr(s0_slot_address LABEL "s0_partition" TARGET ${DEFAULT_IMAGE} ABSOLUTE REQUIRED)
        dt_partition_addr(s1_slot_address LABEL "s1_partition" TARGET ${DEFAULT_IMAGE} ABSOLUTE REQUIRED)
        set(s0_arg --s0-addr ${s0_slot_address})
        set(s1_arg --s1-addr ${s1_slot_address})

        set(bl_storage_target b0)
      endif()
    else(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
      # This is MCUboot downgrade prevention, so we pick the address from MCUboot config.
      set(bl_storage_target mcuboot)
    endif()

    dt_nodelabel(bl_storage_path NODELABEL bl_storage REQUIRED TARGET ${bl_storage_target})

    if(${bl_storage_path} MATCHES /uicr)
      dt_reg_addr(provision_address PATH "${bl_storage_path}" TARGET ${bl_storage_target})
    else()
      dt_partition_addr(provision_address PATH "${bl_storage_path}" TARGET ${bl_storage_target} ABSOLUTE REQUIRED)
    endif()

    dt_reg_size(provision_size PATH "${bl_storage_path}" TARGET ${bl_storage_target})
  endif()

  if(CONFIG_SECURE_BOOT)
    if(cpunet_target)
      # B0 is secure bootloader and we pick the address from its configuration.
      set(bl_storage_target b0n)
    else()
      set(bl_storage_target b0)
    endif()
  else(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    # This is MCUboot downgrade prevention, so we pick the address from MCUboot config.
    set(bl_storage_target mcuboot)
  endif()

  dt_nodelabel(bl_storage_path NODELABEL bl_storage REQUIRED TARGET ${bl_storage_target})

  if(${bl_storage_path} MATCHES /uicr)
    dt_reg_addr(provision_address PATH "${bl_storage_path}" TARGET ${bl_storage_target})
  else()
    dt_partition_addr(provision_address PATH "${bl_storage_path}" TARGET ${bl_storage_target} ABSOLUTE REQUIRED)
  endif()

  dt_reg_size(provision_size PATH "${bl_storage_path}" TARGET ${bl_storage_target})

  if(CONFIG_SECURE_BOOT)
    add_custom_command(
      OUTPUT
      ${PROVISION_HEX}
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/provision.py
      ${s0_arg}
      ${s1_arg}
      --provision-addr ${provision_address}
      ${public_keys_file_arg}
      --output ${PROVISION_HEX}
      --max-size ${provision_size}
      ${monotonic_counter_arg}
      ${no_verify_hashes_arg}
      ${mcuboot_counters_slots}
      --otp-write-width ${otp_write_width}
      ${psa_certificate_reference}
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
      --provision-addr ${provision_address}
      --output ${PROVISION_HEX}
      --max-size ${provision_size}
      ${mcuboot_counters_num}
      ${mcuboot_counters_slots}
      --otp-write-width ${otp_write_width}
      ${psa_certificate_reference}
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
    ALL
    DEPENDS
    ${PROVISION_HEX}
  )

  if(SB_CONFIG_MERGED_HEX_FILES)
    if(cpunet_target)
      sysbuild_get(board_target IMAGE b0n VAR CONFIG_BOARD_TARGET KCONFIG)
    else()
      sysbuild_get(board_target IMAGE b0 VAR CONFIG_BOARD_TARGET KCONFIG)
    endif()

    string(REPLACE "/" "_" board_target ${board_target})
    string(REPLACE "@" "_" board_target ${board_target})
    set_property(GLOBAL APPEND
      PROPERTY sysbuild_merged_hex_dependencies_${board_target} ${prefix_name}provision_target
    )
  endif()
endfunction()

if(SB_CONFIG_SECURE_BOOT)
  b0_gen_keys()
endif()

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

  # b0n and provision data need to be merged into a single hex to allow for it to be flashed
  # without one erasing the other, due to provision data being placed part the way in the b0n's
  # sector which would result in erasing existing data with separate hex files
  sysbuild_get(b0n_kernel_bin_name IMAGE b0n VAR CONFIG_KERNEL_BIN_NAME KCONFIG)

  add_custom_command(
    OUTPUT
      ${APPLICATION_BINARY_DIR}/b0n_provision_merged.hex
    COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/mergehex.py
      -o ${APPLICATION_BINARY_DIR}/b0n_provision_merged.hex
      ${CMAKE_BINARY_DIR}/net_provision.hex
      ${CMAKE_BINARY_DIR}/b0n/zephyr/${b0n_kernel_bin_name}.hex
    DEPENDS
      b0n_extra_byproducts
      ${CMAKE_BINARY_DIR}/b0n/zephyr/${b0n_kernel_bin_name}.hex
      net_provision_target
      ${CMAKE_BINARY_DIR}/net_provision.hex
    WORKING_DIRECTORY
      ${APPLICATION_BINARY_DIR}
  )

  add_custom_target(
    ${slot}_combined_app_provision_hex_target
    ALL
    DEPENDS
      ${APPLICATION_BINARY_DIR}/b0n_provision_merged.hex
  )
endif()
