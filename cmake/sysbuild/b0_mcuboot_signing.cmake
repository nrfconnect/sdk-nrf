# Copyright (c) 2020-2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This file includes extra build system logic that is enabled when
# CONFIG_BOOTLOADER_MCUBOOT=y.
#
# It builds signed binaries using imgtool as a post-processing step
# after zephyr/zephyr.elf is created in the build directory.
#
# Since this file is brought in via include(), we do the work in a
# function to avoid polluting the top-level scope.

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

function(ncs_secure_boot_mcuboot_sign application bin_files signed_targets prefix)
  find_program(IMGTOOL imgtool.py HINTS ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/ NAMES imgtool NAMES_PER_DIR)
  set(keyfile "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  string(CONFIGURE "${keyfile}" keyfile)

  # No imgtool, no signed binaries.
  if(NOT DEFINED IMGTOOL)
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool. To fix, install imgtool with pip3, or add the mcuboot repository to the west manifest and ensure it has a scripts/imgtool.py file.")
    return()
  endif()

  sysbuild_get(application_image_dir IMAGE ${application} VAR APPLICATION_BINARY_DIR CACHE)
  sysbuild_get(CONFIG_BUILD_OUTPUT_BIN IMAGE ${application} VAR CONFIG_BUILD_OUTPUT_BIN KCONFIG)
  sysbuild_get(CONFIG_BUILD_OUTPUT_HEX IMAGE ${application} VAR CONFIG_BUILD_OUTPUT_HEX KCONFIG)

  set(slot_size)        # imgtool --slot-size
  set(header_size)      # imgtool --header
  if(SB_CONFIG_PARTITION_MANAGER)
    string(TOUPPER "${application}" application_uppercase)
    set(slot_size $<TARGET_PROPERTY:partition_manager,${prefix}PM_${application_uppercase}_SIZE>)
    set(header_size ${SB_CONFIG_PM_MCUBOOT_PAD})
  else()
    # With partition manager disabled we need to map applications to partitions by hand as there
    # is no reflection in application name in partitions. This mapping should probably by done
    # at application cmake level.
    set(part_label)
    if(application STREQUAL "mcuboot")
      set(part_label "s0_slot")
    elseif(application STREQUAL "s1_image")
      set(part_label "s1_slot")
    else()
      message(FATAL_ERROR "No mapping for ${application}")
    endif()

    # Get the partition node and pick size from it.
    dt_partition_size(slot_size LABEL "${part_label}" TARGET ${application} REQUIRED)
    # Header size is picked from the image that is beeing signed.
    sysbuild_get(header_size IMAGE mcuboot VAR CONFIG_NCS_MCUBOOT_IMAGE_HEADER_SIZE KCONFIG)
  endif()
  set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version ${SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION} --align 4 --slot-size ${slot_size} --pad-header --header-size ${header_size})

  if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    set(imgtool_extra --security-counter ${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE})
  else()
    set(imgtool_extra)
  endif()

  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_extra -k "${keyfile}" ${imgtool_extra})
  endif()

  # Extensionless prefix of any output file.
  set(output ${CMAKE_BINARY_DIR}/signed_by_mcuboot_and_b0_${application})

  # List of additional build byproducts.
  set(byproducts)

  # 'west sign' arguments for confirmed, unconfirmed and encrypted images.
  set(encrypted_args)

  if(SB_CONFIG_SOC_SERIES_NRF54L AND SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
    if(NOT SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE)
      set(imgtool_extra --sha 512 ${imgtool_extra})
    else()
      set(imgtool_extra --pure ${imgtool_extra})
    endif()
  endif()

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
      ${imgtool_sign} ${imgtool_extra} ${CMAKE_BINARY_DIR}/signed_by_b0_${application}.bin ${output}.bin

      DEPENDS
      ${application}_extra_byproducts
      ${application}_signed_kernel_hex_target
      ${application_image_dir}/zephyr/.config
      ${CMAKE_BINARY_DIR}/signed_by_b0_${application}.bin
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
      ${imgtool_sign} ${imgtool_extra} ${CMAKE_BINARY_DIR}/signed_by_b0_${application}.hex ${output}.hex

      DEPENDS
      ${application}_extra_byproducts
      ${application}_signed_kernel_hex_target
      ${application_image_dir}/zephyr/.config
      ${CMAKE_BINARY_DIR}/signed_by_b0_${application}.hex
      )

    if(SB_CONFIG_PARTITION_MANAGER AND NOT prefix)
      # Set the partition manager hex file and target
      set_property(
        GLOBAL PROPERTY
        ${application}_PM_HEX_FILE
        ${output}.hex
      )

      set_property(
        GLOBAL PROPERTY
        ${application}_PM_TARGET
        ${application}_signed_packaged_target
      )
    endif()
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

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  if(SB_CONFIG_SECURE_BOOT_APPCORE)
    set(bin_files)
    set(signed_targets)
    # extra_bin_data is used for generating zip file, and is set to information
    # on variant S1 image, in case it is also added to a zip file.
    set(extra_bin_data)

    ncs_secure_boot_mcuboot_sign(mcuboot "${bin_files}" "${signed_targets}" "")

    # Becuase S0/S1 images are build for slot they are designated for, we need
    # slot addresses to be able to sign them for that specific slot.
    # We also need slot, for signing sript.
    set(s0_slot_address)
    set(s0_slot_size)
    set(s1_slot_address)
    set(s1_slot_size)
    if(SB_CONFIG_PARTITION_MANAGER)
      set(s1_slot_address "$<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>")
      set(s0_slot_address "$<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>")
    else()
      # The same DTS is used for all images, so the application selection does not
      # matter here, so we just use the mcuboot
      dt_partition_addr(s0_slot_address LABEL "s0_slot" TARGET mcuboot ABSOLUTE REQUIRED)
      dt_partition_size(s0_slot_size LABEL "s0_slot" TARGET mcuboot REQUIRED)
      dt_partition_addr(s1_slot_address LABEL "s1_slot" TARGET mcuboot ABSOLUTE REQUIRED)
      dt_partition_size(s1_slot_size LABEL "s1_slot" TARGET mcuboot REQUIRED)
    endif()

    # Signing the MCUboot image, secondary stage bootloader, that will be running from S1 slot.
    if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
      ncs_secure_boot_mcuboot_sign(s1_image "${bin_files}" "${signed_targets}" "")
      set(extra_bin_data "signed_by_mcuboot_and_b0_s1_image.binload_address=${s1_slot_address};signed_by_mcuboot_and_b0_s1_image.binslot=1")
    endif()

    if(bin_files)
      sysbuild_get(mcuboot_fw_info_firmware_version IMAGE mcuboot VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)

      include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

      generate_dfu_zip(
        OUTPUT ${CMAKE_BINARY_DIR}/dfu_mcuboot.zip
        BIN_FILES ${bin_files}
        TYPE mcuboot
        IMAGE mcuboot
        SCRIPT_PARAMS
        "signed_by_mcuboot_and_b0_mcuboot.binload_address=${s0_slot_address}"
        ${extra_bin_data}
        "version_MCUBOOT=${SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION}"
        "version_B0=${mcuboot_fw_info_firmware_version}"
        "signed_by_mcuboot_and_b0_mcuboot.binslot=0"
        DEPENDS ${signed_targets}
        )
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_CPUNET)
    ncs_secure_boot_mcuboot_sign(${image_name} "${bin_files}" "${signed_targets}" CPUNET_)
  endif()

  # Clear temp variables
  set(image_name)
  set(bin_files)
  set(signed_targets)
  set(extra_bin_data)
endif()
