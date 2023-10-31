# Copyright (c) 2020-2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This file includes extra build system logic that is enabled when
# CONFIG_BOOTLOADER_MCUBOOT=y.
#
# It builds signed binaries using imgtool as a post-processing step
# after zephyr/zephyr.elf is created in the build directory.
#
# Since this file is brought in via include(), we do the work in a
# function to avoid polluting the top-level scope.

function(ncs_secure_boot_mcuboot_sign application application_name bin_files signed_targets)
  set(keyfile "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")

  # Find imgtool. Even though west is installed, imgtool might not be.
  # The user may also have a custom manifest which doesn't include
  # MCUboot.
  #
  # Therefore, go with an explicitly installed imgtool first, falling
  # back on mcuboot/scripts/imgtool.py.
  if(IMGTOOL)
    set(imgtool_path "${IMGTOOL}")
  elseif(DEFINED ZEPHYR_MCUBOOT_MODULE_DIR)
    set(IMGTOOL_PY "${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/imgtool.py")
    if(EXISTS "${IMGTOOL_PY}")
      set(imgtool_path "${IMGTOOL_PY}")
    endif()
  endif()

  # No imgtool, no signed binaries.
  if(NOT DEFINED imgtool_path)
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool. To fix, install imgtool with pip3, or add the mcuboot repository to the west manifest and ensure it has a scripts/imgtool.py file.")
    return()
  endif()

  sysbuild_get(application_image_dir IMAGE ${application} VAR APPLICATION_BINARY_DIR CACHE)
  sysbuild_get(CONFIG_BUILD_OUTPUT_BIN IMAGE ${application} VAR CONFIG_BUILD_OUTPUT_BIN KCONFIG)
  sysbuild_get(CONFIG_BUILD_OUTPUT_HEX IMAGE ${application} VAR CONFIG_BUILD_OUTPUT_HEX KCONFIG)

  string(TOUPPER "${application_name}" application_name_uppercase)
  set(imgtool_sign ${imgtool_path} sign --version ${SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION} --align 4 --slot-size $<TARGET_PROPERTY:partition_manager,PM_${application_name_uppercase}_SIZE> --pad-header --header-size ${SB_CONFIG_PM_MCUBOOT_PAD})

  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_extra -k "${keyfile}" ${imgtool_extra})
  endif()

  # Extensionless prefix of any output file.
  set(output ${PROJECT_BINARY_DIR}/signed_by_mcuboot_and_b0_${application})

  # List of additional build byproducts.
  set(byproducts)

  # 'west sign' arguments for confirmed, unconfirmed and encrypted images.
  set(encrypted_args)

  # Set up .bin outputs.
  if(CONFIG_BUILD_OUTPUT_BIN)
    list(APPEND byproducts ${output}.bin)
    list(APPEND bin_files ${output}.bin)
    set(bin_files ${bin_files} PARENT_SCOPE)

      add_custom_command(
        OUTPUT
        ${output}.bin # Signed hex with IMAGE_MAGIC located at secondary slot

        COMMAND
        # Create version of test update which is located at the secondary slot.
        # Hence, if a programmer is given this hex file, it will flash it
        # to the secondary slot, and upon reboot mcuboot will swap in the
        # contents of the hex file.
        ${imgtool_sign} ${PROJECT_BINARY_DIR}/signed_by_b0_${application}.bin ${output}.bin

        DEPENDS
        ${application}_extra_byproducts
        ${application}_signed_kernel_hex_target
        ${application_image_dir}/zephyr/.config
        ${PROJECT_BINARY_DIR}/signed_by_b0_${application}.bin
        )
  endif()

  # Set up .hex outputs.
  if(CONFIG_BUILD_OUTPUT_HEX)
    list(APPEND byproducts ${output}.hex)

      add_custom_command(
        OUTPUT
        ${output}.hex # Signed hex with IMAGE_MAGIC located at secondary slot

        COMMAND
        # Create version of test update which is located at the secondary slot.
        # Hence, if a programmer is given this hex file, it will flash it
        # to the secondary slot, and upon reboot mcuboot will swap in the
        # contents of the hex file.
        ${imgtool_sign} ${PROJECT_BINARY_DIR}/signed_by_b0_${application}.hex ${output}.hex

        DEPENDS
        ${application}_extra_byproducts
        ${application}_signed_kernel_hex_target
        ${application_image_dir}/zephyr/.config
        ${PROJECT_BINARY_DIR}/signed_by_b0_${application}.hex
        )
  endif()

  # Add the west sign calls and their byproducts to the post-processing
  # steps for zephyr.elf.
  #
  # CMake guarantees that multiple COMMANDs given to
  # add_custom_command() are run in order, so adding the 'west sign'
  # calls to the "extra_post_build_commands" property ensures they run
  # after the commands which generate the unsigned versions.

  if(byproducts)
    add_custom_target(${application}_signed_packaged_target
        ALL DEPENDS
        ${byproducts}
        )

    list(APPEND signed_targets ${application}_signed_packaged_target)
    set(signed_targets ${signed_targets} PARENT_SCOPE)
  endif()
endfunction()

if(SB_CONFIG_BOOTLOADER_MCUBOOT AND SB_CONFIG_SECURE_BOOT)
  set(bin_files)
  set(signed_targets)

  if(SB_CONFIG_SECURE_BOOT_APPCORE)
    set(s0_image_name "mcuboot")
    set(s0_application_name "${s0_image_name}")
  else()
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_CPUNET)
    set(s0_image_name "${image_name}")
    set(s0_application_name "mcuboot_primary_1")
  endif()

  ncs_secure_boot_mcuboot_sign("${s0_image_name}" "${s0_application_name}" "${bin_files}" "${signed_targets}")

  if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
    ncs_secure_boot_mcuboot_sign("s1_image" "s1_image" "${bin_files}" "${signed_targets}")
  endif()

  if(bin_files)
    sysbuild_get(mcuboot_fw_info_firmware_version IMAGE mcuboot VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)

    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

    generate_dfu_zip(
      OUTPUT ${PROJECT_BINARY_DIR}/dfu_mcuboot.zip
      BIN_FILES ${bin_files}
      TYPE mcuboot
      IMAGE mcuboot
      SCRIPT_PARAMS
      "signed_by_mcuboot_and_b0_${s0_image_name}.binload_address=$<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>"
      "signed_by_mcuboot_and_b0_s1_image.binload_address=$<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>"
      "version_MCUBOOT=${SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION}"
      "version_B0=${mcuboot_fw_info_firmware_version}"
      DEPENDS ${signed_targets}
      )
  endif()

  # Clear temp variables
  set(bin_files)
  set(signed_targets)
endif()
