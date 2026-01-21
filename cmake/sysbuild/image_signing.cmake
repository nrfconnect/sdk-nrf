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

function(zephyr_runner_file type path)
  # Property magic which makes west flash choose the signed build
  # output of a given type.
  set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
endfunction()

function(zephyr_mcuboot_tasks)
  set(keyfile "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}")
  set(keyfile_enc "${CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE}")
  string(CONFIGURE "${keyfile}" keyfile)
  string(CONFIGURE "${keyfile_enc}" keyfile_enc)

  if(NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}")
    # Check for misconfiguration.
    if("${keyfile}" STREQUAL "")
      # No signature key file, no signed binaries. No error, though:
      # this is the documented behavior.
      message(WARNING "Neither CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE or "
                      "CONFIG_MCUBOOT_SIGNATURE_KEY_FILE are set, the generated build will not be "
                      "bootable by MCUboot unless it is signed manually/externally.")
      return()
    endif()
  endif()

  foreach(file keyfile keyfile_enc)
    if(NOT "${${file}}" STREQUAL "")
      if(NOT IS_ABSOLUTE "${${file}}")
        # Relative paths are relative to 'west topdir'.
        set(${file} "${WEST_TOPDIR}/${${file}}")
      endif()

      if(NOT EXISTS "${${file}}" AND NOT "${CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE}")
        message(FATAL_ERROR "west sign can't find file ${${file}} (Note: Relative paths are relative to the west workspace topdir \"${WEST_TOPDIR}\")")
      elseif(NOT (CONFIG_BUILD_OUTPUT_BIN OR CONFIG_BUILD_OUTPUT_HEX))
        message(FATAL_ERROR "Can't sign images for MCUboot: Neither CONFIG_BUILD_OUTPUT_BIN nor CONFIG_BUILD_OUTPUT_HEX is enabled, so there's nothing to sign.")
      endif()
    endif()
  endforeach()

  # No imgtool, no signed binaries.
  if(NOT DEFINED IMGTOOL)
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool. To fix, install imgtool with pip3, or add the mcuboot repository to the west manifest and ensure it has a scripts/imgtool.py file.")
    return()
  endif()

  # Fetch devicetree details for flash and slot information
  dt_chosen(flash_node PROPERTY "zephyr,flash")
  dt_nodelabel(slot0_flash NODELABEL "slot0_partition" REQUIRED)
  dt_reg_size(slot_size PATH "${slot0_flash}" REQUIRED)
  dt_prop(write_block_size PATH "${flash_node}" PROPERTY "write-block-size")

  if(NOT write_block_size)
    set(write_block_size 4)
    message(WARNING "slot0_partition write block size devicetree parameter is missing, assuming write block size is 4")
  endif()

  if(CONFIG_PARTITION_MANAGER_ENABLED)
    if(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT OR CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP)
      # XIP image, need to use the fixed address for this slot
      if(CONFIG_NCS_IS_VARIANT_IMAGE)
        set(imgtool_rom_command --rom-fixed @PM_MCUBOOT_SECONDARY_ADDRESS@)
      else()
        set(imgtool_rom_command --rom-fixed @PM_MCUBOOT_PRIMARY_ADDRESS@)
      endif()
    endif()

    # Split fields, imgtool_sign_sysbuild is stored in cache which will have fields updated by
    # sysbuild, imgtool_sign must not be stored in cache because it would then prevent those fields
    # from being updated without a pristine build
    # TODO: NCSDK-28461 sysbuild PM fields cannot be updated without a pristine build, will become
    # invalid if a static PM file is updated without pristine build
    set(imgtool_sign_sysbuild --slot-size @PM_MCUBOOT_PRIMARY_SIZE@ --pad-header --header-size @PM_MCUBOOT_PAD_SIZE@ ${imgtool_rom_command} CACHE STRING "imgtool sign sysbuild replacement")
    set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --align ${write_block_size} ${imgtool_sign_sysbuild})
  else()
    set(imgtool_rom_command)
    if(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT OR CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP)
      dt_chosen(code_partition PROPERTY "zephyr,code-partition")
      dt_partition_addr(code_partition_offset PATH "${code_partition}" REQUIRED)
      dt_reg_size(slot_size PATH "${code_partition}" REQUIRED)
      set(imgtool_rom_command --rom-fixed ${code_partition_offset})
    endif()
    set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --align ${write_block_size} --slot-size ${slot_size} --header-size ${CONFIG_ROM_START_OFFSET} ${imgtool_rom_command})
  endif()

  # Arguments to imgtool.
  if(NOT CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS STREQUAL "")
    # Separate extra arguments into the proper format for adding to
    # extra_post_build_commands.
    #
    # Use UNIX_COMMAND syntax for uniform results across host
    # platforms.
    separate_arguments(imgtool_extra UNIX_COMMAND ${CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS})
  else()
    set(imgtool_extra)
  endif()

  if(CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT_ENABLED)
    set(imgtool_bin_extra --compression lzma2armthumb)
  else()
    set(imgtool_bin_extra)
  endif()

  # Apply compression to hex file if this is a test
  if(ncs_compress_test_compress_hex)
    set(imgtool_hex_extra ${imgtool_bin_extra})
  else()
    set(imgtool_hex_extra)
  endif()

  # Set proper hash calculation algorithm for signing
  if(CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE)
    set(imgtool_extra --pure ${imgtool_extra})
  elseif(CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512)
    set(imgtool_extra --sha 512 ${imgtool_extra})
  endif()

  if(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    set(imgtool_extra --security-counter ${CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE} ${imgtool_extra})
  endif()

  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_extra -k "${keyfile}" ${imgtool_extra})
  endif()

  if(CONFIG_MCUBOOT_IMGTOOL_UUID_VID)
    set(imgtool_extra ${imgtool_extra} --vid "${CONFIG_MCUBOOT_IMGTOOL_UUID_VID_NAME}")
  endif()

  if(CONFIG_MCUBOOT_IMGTOOL_UUID_CID)
    set(imgtool_extra ${imgtool_extra} --cid "${CONFIG_MCUBOOT_IMGTOOL_UUID_CID_NAME}")
  endif()

  if(CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST)
    cmake_path(GET CMAKE_BINARY_DIR PARENT_PATH sysbuild_binary_dir)
    if(CONFIG_NCS_IS_VARIANT_IMAGE)
      cmake_path(APPEND APPLICATION_CONFIG_DIR "mcuboot_manifest_secondary.yaml" OUTPUT_VARIABLE manifest_path)
      if(NOT EXISTS "${manifest_path}")
        cmake_path(APPEND sysbuild_binary_dir "mcuboot_manifest_secondary.yaml" OUTPUT_VARIABLE manifest_path)
      endif()
    else()
      cmake_path(APPEND APPLICATION_CONFIG_DIR "mcuboot_manifest.yaml" OUTPUT_VARIABLE manifest_path)
      if(NOT EXISTS "${manifest_path}")
        cmake_path(APPEND sysbuild_binary_dir "mcuboot_manifest.yaml" OUTPUT_VARIABLE manifest_path)
      endif()
    endif()
    set(imgtool_extra ${imgtool_extra} --manifest "${manifest_path}")
  endif()

  set(imgtool_args ${imgtool_extra})

  # Extensionless prefix of any output file.
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed)

  if(CONFIG_BUILD_WITH_TFM)
    set(input ${APPLICATION_BINARY_DIR}/zephyr/tfm_merged)
  else()
    set(input ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_NAME})
  endif()

  # List of additional build byproducts.
  set(byproducts)

  # Input file to sign
  set(input_arg)
  # Additional (algorithm-dependent) args for encryption
  set(imgtool_encrypt_extra_args)

  if(NOT "${keyfile_enc}" STREQUAL "")
    if(CONFIG_MCUBOOT_ENCRYPTION_ALG_AES_256)
      set(imgtool_args ${imgtool_args} --encrypt-keylen 256)
    endif()

    # Signature type determines key exchange scheme; ED25519 here means
    # ECIES-X25519 is used. Default to HMAC-SHA512 for ECIES-X25519.
    # Only .encrypted.bin file gets the ENCX25519/ENCX25519_SHA512, the
    # just signed one does not.
    # Only NRF54L gets the HMAC-SHA512, other remain with previously used
    # SHA256.
    if(CONFIG_SOC_SERIES_NRF54L AND CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_ED25519)
      set(imgtool_encrypt_extra_args --hmac-sha 512)
    endif()
  endif()

  # Set up .bin outputs.
  if(CONFIG_BUILD_OUTPUT_BIN)
    if(CONFIG_BUILD_WITH_TFM)
      # TF-M does not generate a bin file, so use the hex file as an input
      set(input_arg ${input}.hex)
    else()
      set(input_arg ${input}.bin)
    endif()

    list(APPEND byproducts ${output}.bin)
    zephyr_runner_file(bin ${output}.bin)
    set(BYPRODUCT_KERNEL_SIGNED_BIN_NAME "${output}.bin"
      CACHE FILEPATH "Signed kernel bin file" FORCE
    )

    # Add the west sign calls and their byproducts to the post-processing
    # steps for zephyr.elf.
    #
    # CMake guarantees that multiple COMMANDs given to
    # add_custom_command() are run in order, so adding the 'west sign'
    # calls to the "extra_post_build_commands" property ensures they run
    # after the commands which generate the unsigned versions.
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${imgtool_sign} ${imgtool_args} ${imgtool_bin_extra} ${input_arg} ${output}.bin)

    if(NOT "${keyfile_enc}" STREQUAL "")
      list(APPEND byproducts ${output}.encrypted.bin)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_BIN_NAME "${output}.encrypted.bin"
        CACHE FILEPATH "Signed and encrypted kernel bin file" FORCE
      )
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_sign} ${imgtool_args} ${imgtool_bin_extra} ${imgtool_encrypt_extra_args} --encrypt
        "${keyfile_enc}" ${input_arg} ${output}.encrypted.bin)
    endif()

    # Generate and use the confirmed image in Direct XIP with revert, so the
    # default application will boot after flashing.
    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE OR CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
      list(APPEND byproducts ${output}.confirmed.bin)
      zephyr_runner_file(bin ${output}.confirmed.bin)
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_BIN_NAME "${output}.confirmed.bin"
        CACHE FILEPATH "Signed and confirmed kernel bin file" FORCE
      )
      if("${keyfile_enc}" STREQUAL "")
        set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
          ${imgtool_sign} ${imgtool_args} ${imgtool_bin_extra} --pad --confirm ${input_arg}
          ${output}.confirmed.bin)
      else()
        set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
          ${imgtool_sign} ${imgtool_args} ${imgtool_bin_extra} ${imgtool_encrypt_extra_args}
          --encrypt "${keyfile_enc}" --clear --pad --confirm ${input_arg} ${output}.confirmed.bin)
      endif()
    endif()
  endif()

  # Set up .hex outputs.
  if(CONFIG_BUILD_OUTPUT_HEX)
    set(input_arg ${input}.hex)
    list(APPEND byproducts ${output}.hex)

    # If using partition manager do not run zephyr_runner_file here as PM will
    # provide the merged hex file from sysbuild's scope unless this is a variant image
    # Otherwise run zephyr_runner_file for both images to ensure the signed hex file
    # is used for flashing
    if((NOT CONFIG_PARTITION_MANAGER_ENABLED) OR CONFIG_NCS_IS_VARIANT_IMAGE)
      zephyr_runner_file(hex ${output}.hex)
    endif()

    set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${output}.hex"
      CACHE FILEPATH "Signed kernel hex file" FORCE
    )

    # Add the west sign calls and their byproducts to the post-processing
    # steps for zephyr.elf.
    #
    # CMake guarantees that multiple COMMANDs given to
    # add_custom_command() are run in order, so adding the 'west sign'
    # calls to the "extra_post_build_commands" property ensures they run
    # after the commands which generate the unsigned versions.
    if("${keyfile_enc}" STREQUAL "")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_sign} ${imgtool_args} ${imgtool_hex_extra} ${input_arg} ${output}.hex)
    else()
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_sign} ${imgtool_args} ${imgtool_hex_extra} ${imgtool_encrypt_extra_args} --encrypt
        "${keyfile_enc}" --clear ${input_arg} ${output}.hex)

      list(APPEND byproducts ${output}.encrypted.hex)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_HEX_NAME "${output}.encrypted.hex"
        CACHE FILEPATH "Signed and encrypted kernel hex file" FORCE
      )

      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_sign} ${imgtool_args} ${imgtool_hex_extra} ${imgtool_encrypt_extra_args} --encrypt
        "${keyfile_enc}" ${input_arg} ${output}.encrypted.hex)
    endif()

    # Generate and use the confirmed image in Direct XIP with revert, so the
    # default application will boot after flashing.
    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE OR CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
      list(APPEND byproducts ${output}.confirmed.hex)
      if((NOT CONFIG_PARTITION_MANAGER_ENABLED) OR CONFIG_NCS_IS_VARIANT_IMAGE)
        zephyr_runner_file(hex ${output}.confirmed.hex)
      endif()
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_HEX_NAME "${output}.confirmed.hex"
        CACHE FILEPATH "Signed and confirmed kernel hex file" FORCE
      )
      if("${keyfile_enc}" STREQUAL "")
        set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
          ${imgtool_sign} ${imgtool_args} ${imgtool_hex_extra} --pad --confirm ${input_arg}
          ${output}.confirmed.hex)
      else()
        set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
          ${imgtool_sign} ${imgtool_args} ${imgtool_hex_extra} ${imgtool_encrypt_extra_args}
          --encrypt "${keyfile_enc}" --clear --pad --confirm ${input_arg} ${output}.confirmed.hex)
      endif()
    endif()
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})
endfunction()

zephyr_mcuboot_tasks()
