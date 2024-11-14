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

function(split)
  # Splits an elf image into 2 sets of hex and bin files
  #
  # Required arguments are:
  # ELF_FILE_IN
  # ELF_SECTION_NAMES
  # HEX_INCLUDE_FILE_OUT
  # HEX_EXCLUDE_FILE_OUT
  set(oneValueArgs ELF_FILE_IN HEX_INCLUDE_FILE_OUT HEX_EXCLUDE_FILE_OUT)
  set(multiValueArgs ELF_SECTION_NAMES)
  cmake_parse_arguments(SPLIT_ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  check_arguments_required_all(split SPLIT_ARG ${oneValueArgs} ELF_SECTION_NAMES)

  set(include_param)
  set(exclude_param)

  foreach(section_name ${SPLIT_ARG_ELF_SECTION_NAMES})
    set(include_param ${include_param} -R ${section_name})
    set(exclude_param ${exclude_param} -j ${section_name})
  endforeach()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
    ${CMAKE_OBJCOPY} --output-target=ihex ${include_param} ${SPLIT_ARG_ELF_FILE_IN} ${SPLIT_ARG_HEX_INCLUDE_FILE_OUT})
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
    ${CMAKE_OBJCOPY} --output-target=ihex ${exclude_param} ${SPLIT_ARG_ELF_FILE_IN} ${SPLIT_ARG_HEX_EXCLUDE_FILE_OUT})
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

  # Fetch devicetree details for flash and slot information
  dt_chosen(flash_node PROPERTY "zephyr,flash")
  dt_nodelabel(slot0_flash NODELABEL "slot0_partition" REQUIRED)
  dt_prop(slot_size PATH "${slot0_flash}" PROPERTY "reg" INDEX 1 REQUIRED)
  dt_prop(write_block_size PATH "${flash_node}" PROPERTY "write-block-size")

  if(NOT write_block_size)
    set(write_block_size 4)
    message(WARNING "slot0_partition write block size devicetree parameter is missing, assuming write block size is 4")
  endif()

  # Fetch QSPI XIP MCUboot image number from sysbuild
  zephyr_get(qspi_xip_image_number VAR QSPI_XIP_IMAGE_NUMBER SYSBUILD)

  if(CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT OR CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP)
    # XIP image, need to use the fixed address for these slots
    if(CONFIG_NCS_IS_VARIANT_IMAGE)
      set(imgtool_internal_rom_command --rom-fixed @PM_MCUBOOT_SECONDARY_ADDRESS@)
      set(imgtool_external_rom_command --rom-fixed @PM_MCUBOOT_SECONDARY_${qspi_xip_image_number}_ADDRESS@)
    else()
      set(imgtool_internal_rom_command --rom-fixed @PM_MCUBOOT_PRIMARY_ADDRESS@)
      set(imgtool_external_rom_command --rom-fixed @PM_MCUBOOT_PRIMARY_${qspi_xip_image_number}_ADDRESS@)
    endif()
  endif()

  # Split fields, imgtool_[internal/external]_sign_sysbuild are stored in cache which will have
  # fields updated by sysbuild, imgtool_[internal/external]_sign must not be stored in cache
  # because it would then prevent those fields from being updated without a pristine build
  # TODO: NCSDK-28461 sysbuild PM fields cannot be updated without a pristine build, will become
  # invalid if a static PM file is updated without pristine build
  set(imgtool_internal_sign_sysbuild --slot-size @PM_MCUBOOT_PRIMARY_SIZE@ --pad-header --header-size @PM_MCUBOOT_PAD_SIZE@ ${imgtool_internal_rom_command} CACHE STRING "imgtool sign (internal flash) sysbuild replacement")
  set(imgtool_external_sign_sysbuild --slot-size @PM_MCUBOOT_PRIMARY_${qspi_xip_image_number}_SIZE@ --pad-header --header-size @PM_MCUBOOT_PAD_SIZE@ ${imgtool_external_rom_command} CACHE STRING "imgtool sign (external flash) sysbuild replacement")
  set(imgtool_internal_sign ${PYTHON_EXECUTABLE} ${imgtool_path} sign --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --align ${write_block_size} ${imgtool_internal_sign_sysbuild})
  set(imgtool_external_sign ${PYTHON_EXECUTABLE} ${imgtool_path} sign --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --align ${write_block_size} ${imgtool_external_sign_sysbuild})

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

  if(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    set(imgtool_extra --security-counter ${CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE} ${imgtool_extra})
  endif()

  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_extra -k "${keyfile}" ${imgtool_extra})
  endif()

  set(imgtool_args ${imgtool_extra})

  # Extensionless prefix of any output file.
  set(output_internal ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.internal.signed)
  set(output_external ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.external.signed)
  set(output_merged ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed)

  if(CONFIG_BUILD_WITH_TFM)
#    set(input ${APPLICATION_BINARY_DIR}/zephyr/tfm_merged)
    message(FATAL_ERROR "TFM is not currently supported with QSPI XIP")
  else()
    set(input_internal ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.internal)
    set(input_external ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.external)
  endif()

  # List of additional build byproducts.
  set(byproducts)

  # 'west sign' arguments for confirmed, unconfirmed and encrypted images.
  set(unconfirmed_args)
  set(confirmed_args)
  set(encrypted_args)

  # Split files apart
  split(
    ELF_FILE_IN ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
    ELF_SECTION_NAMES ".extflash_text_reloc;.extflash_rodata_reloc"
    HEX_INCLUDE_FILE_OUT ${input_internal}.hex
    HEX_EXCLUDE_FILE_OUT ${input_external}.hex
  )

  # Set up .hex outputs.
  if(CONFIG_BUILD_OUTPUT_HEX)
    set(unconfirmed_internal_args ${input_internal}.hex ${output_internal}.hex)
    set(unconfirmed_external_args ${input_external}.hex ${output_external}.hex)
    list(APPEND byproducts ${output_internal}.hex ${output_external}.hex)

    # Do not run zephyr_runner_file here as PM will provide the merged hex file from
    # sysbuild's scope unless this is a variant image
    if(CONFIG_NCS_IS_VARIANT_IMAGE)
      zephyr_runner_file(hex ${output_merged}.hex)
    endif()

    set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${output_merged}.hex"
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
      ${imgtool_internal_sign} ${imgtool_args} ${unconfirmed_internal_args})
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${imgtool_external_sign} ${imgtool_args} ${unconfirmed_external_args})

    # Combine the signed hex files into a single output hex file
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py -o ${output_merged}.hex ${output_internal}.hex ${output_external}.hex)

    if(NOT "${keyfile_enc}" STREQUAL "")
      set(unconfirmed_internal_args ${input_internal}.hex ${output_internal}.encrypted.hex)
      set(unconfirmed_external_args ${input_external}.hex ${output_external}.encrypted.hex)
      list(APPEND byproducts ${output_internal}.encrypted.hex ${output_external}.encrypted.hex)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_HEX_NAME "${output_merged}.encrypted.hex"
          CACHE FILEPATH "Signed and encrypted kernel hex file" FORCE
      )

      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_internal_sign} ${imgtool_args} --encrypt "${keyfile_enc}" ${unconfirmed_internal_args})
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${imgtool_external_sign} ${imgtool_args} --encrypt "${keyfile_enc}" ${unconfirmed_external_args})

      # Combine the signed hex files into a single output hex file
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py -o ${output_merged}.encrypted.hex ${output_internal}.hex ${output_external}.hex)
    endif()
  endif()

  # Set up .bin outputs.
  if(CONFIG_BUILD_OUTPUT_BIN)
    # Instead of re-signing, convert the hex files to binary using objcopy
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${CMAKE_OBJCOPY} --input-target=ihex --output-target=binary ${output_internal}.hex ${output_internal}.bin)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
      ${CMAKE_OBJCOPY} --input-target=ihex --output-target=binary ${output_external}.hex ${output_external}.bin)

    list(APPEND byproducts "${output_internal}.bin;${output_external}.bin")

    set(BYPRODUCT_KERNEL_SIGNED_BIN_NAME "${output_internal}.bin;${output_external}.bin"
        CACHE FILEPATH "Signed kernel bin files" FORCE
    )

    if(NOT "${keyfile_enc}" STREQUAL "")
      # Instead of re-signing, convert the encrypted hex files to binary using objcopy
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${CMAKE_OBJCOPY} --input-target=ihex --output-target=binary ${output_internal}.encrypted.hex ${output_internal}.encrypted.bin)
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
        ${CMAKE_OBJCOPY} --input-target=ihex --output-target=binary ${output_external}.encrypted.hex ${output_external}.encrypted.bin)

      list(APPEND byproducts "${output_internal}.encrypted.bin;${output_external}.encrypted.bin")
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_BIN_NAME "${output_internal}.encrypted.bin;${output_external}.encrypted.bin"
          CACHE FILEPATH "Signed and encrypted kernel bin files" FORCE
      )
    endif()
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${byproducts})
endfunction()

zephyr_mcuboot_tasks()
