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

function(zephyr_runner_file type path)
  # Property magic which makes west flash choose the signed build
  # output of a given type.
  set_target_properties(runners_yaml_props_target PROPERTIES "${type}_file" "${path}")
endfunction()

function(zephyr_mcuboot_tasks)
  set(keyfile "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}")
  set(keyfile_enc "${CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE}")

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
  dt_prop(slot_size PATH "${slot0_flash}" PROPERTY "reg" INDEX 1 REQUIRED)
  dt_prop(write_block_size PATH "${flash_node}" PROPERTY "write-block-size")

  if(NOT write_block_size)
    set(write_block_size 4)
    message(WARNING "slot0_partition write block size devicetree parameter is missing, assuming write block size is 4")
  endif()

  # Split fields, imgtool_sign_sysbuild is stored in cache which will have fields updated by
  # sysbuild, imgtool_sign must not be stored in cache because it would then prevent those fields
  # from being updated without a pristine build
  # TODO: NCSDK-28461 sysbuild PM fields cannot be updated without a pristine build, will become
  # invalid if a static PM file is updated without pristine build
  set(imgtool_rom_command --rom-fixed @PM_MCUBOOT_SECONDARY_ADDRESS@)
  set(imgtool_sign_sysbuild --slot-size @PM_MCUBOOT_SECONDARY_SIZE@ --pad-header --header-size @PM_MCUBOOT_PAD_SIZE@ ${imgtool_rom_command} CACHE STRING "imgtool sign sysbuild replacement")
  set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --align ${write_block_size} ${imgtool_sign_sysbuild})

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

  # Set proper hash calculation algorithm for signing
  if(CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE)
    set(imgtool_extra --pure ${imgtool_extra})
  elseif(CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512)
    set(imgtool_extra --sha 512 ${imgtool_extra})
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

  set(imgtool_args ${imgtool_extra})

  # Extensionless prefix of any output file.
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed)

  set(input ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_NAME})

  # List of additional build byproducts.
  set(byproducts)

  # 'west sign' arguments for confirmed, unconfirmed and encrypted images.
  set(unconfirmed_args)
  set(confirmed_args)
  set(encrypted_args)

  if(NOT "${keyfile_enc}" STREQUAL "")
    if(CONFIG_MCUBOOT_ENCRYPTION_ALG_AES_256)
      set(imgtool_args ${imgtool_args} --encrypt-keylen 256)
    endif()
  endif()

  # Set up .bin outputs.
  if(CONFIG_BUILD_OUTPUT_BIN)
    if(CONFIG_BUILD_WITH_TFM)
      # TF-M does not generate a bin file, so use the hex file as an input
      set(unconfirmed_args ${input}.hex ${output}.bin)
    else()
      set(unconfirmed_args ${input}.bin ${output}.bin)
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
      ${imgtool_sign} ${imgtool_args} ${unconfirmed_args})

    if(NOT "${keyfile_enc}" STREQUAL "")
      if(CONFIG_BUILD_WITH_TFM)
        # TF-M does not generate a bin file, so use the hex file as an input
        set(unconfirmed_args ${input}.hex ${output}.encrypted.bin)
      else()
        set(unconfirmed_args ${input}.bin ${output}.encrypted.bin)
      endif()

      list(APPEND byproducts ${output}.encrypted.bin)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_BIN_NAME "${output}.encrypted.bin"
          CACHE FILEPATH "Signed and encrypted kernel bin file" FORCE
      )

      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_sign} ${imgtool_args} --encrypt "${keyfile_enc}" ${unconfirmed_args})
    endif()
  endif()

  # Set up .hex outputs.
  if(CONFIG_BUILD_OUTPUT_HEX)
    set(unconfirmed_args ${input}.hex ${output}.hex)
    list(APPEND byproducts ${output}.hex)

    # Do not run zephyr_runner_file here as PM will provide the merged hex file from
    # sysbuild's scope unless this is a variant image
    if(CONFIG_NCS_IS_VARIANT_IMAGE)
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
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${imgtool_sign} ${imgtool_args} ${unconfirmed_args})

    if(NOT "${keyfile_enc}" STREQUAL "")
      set(unconfirmed_args ${input}.hex ${output}.encrypted.hex)
      list(APPEND byproducts ${output}.encrypted.hex)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_HEX_NAME "${output}.encrypted.hex"
          CACHE FILEPATH "Signed and encrypted kernel hex file" FORCE
      )

      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_sign} ${imgtool_args} --encrypt "${keyfile_enc}" ${unconfirmed_args})
    endif()
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})
endfunction()

zephyr_mcuboot_tasks()
