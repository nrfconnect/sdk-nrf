# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

function(merge_images_nrf54h20 output_artifact images)
  find_program(MERGEHEX mergehex.py HINTS ${ZEPHYR_BASE}/scripts/build/ NAMES
    mergehex NAMES_PER_DIR)
  if(NOT DEFINED MERGEHEX)
    message(FATAL_ERROR "Can't merge images: can't find mergehex.py")
    return()
  endif()

  foreach(image ${images})
    # Build a dependency list for the final (merged) artifact.
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_HEX_FILE IMAGE ${image} VAR RUNNERS_HEX_FILE_TO_MERGE
      CACHE)
    cmake_path(APPEND BINARY_DIR "zephyr" ${BINARY_HEX_FILE} OUTPUT_VARIABLE
      app_binary)
    list(APPEND binaries_to_merge "${app_binary}")
  endforeach()

  get_filename_component(merge_target ${output_artifact} NAME_WE)
  add_custom_target(
    merged_${merge_target}
    ${PYTHON_EXECUTABLE} ${MERGEHEX}
      -o ${output_artifact} ${binaries_to_merge}
    DEPENDS
    ${binaries_to_merge}
    BYPRODUCTS
    ${output_artifact}
    COMMENT "Merge intermediate images"
  )
endfunction()

function(disable_programming_nrf54h20 images)
  foreach(image ${images})
    set_target_properties(${image} PROPERTIES BUILD_ONLY true)
  endforeach()
endfunction()

function(mcuboot_sign_merged_nrf54h20 merged_hex main_image)
  find_program(IMGTOOL imgtool.py HINTS ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/
    NAMES imgtool NAMES_PER_DIR)
  find_program(HEX2BIN hex2bin.py NAMES hex2bin)
  set(keyfile "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  set(keyfile_enc "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")
  set(imgtool_cmd)
  string(CONFIGURE "${keyfile}" keyfile)
  string(CONFIGURE "${keyfile_enc}" keyfile_enc)

  if(NOT "${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
    # Check for misconfiguration.
    if("${keyfile}" STREQUAL "")
      # No signature key file, no signed binaries. No error, though:
      # this is the documented behavior.
      message(WARNING "Neither SB_CONFIG_SIGNATURE_TYPE or "
                      "SB_CONFIG_BOOT_SIGNATURE_KEY_FILE are set, the generated"
                      " build will not be bootable by MCUboot unless it is "
                      "signed manually/externally.")
      return()
    endif()

    foreach(file keyfile keyfile_enc)
      if("${${file}}" STREQUAL "")
        continue()
      endif()

      # Find the key files in the order of preference for a simple search
      # modeled by the if checks across the various locations
      #
      #  1. absolute
      #  2. application config
      #  3. west topdir (optional when the workspace is not west managed)
      #
      if(NOT IS_ABSOLUTE "${${file}}")
        if(EXISTS "${SB_APPLICATION_CONFIG_DIR}/${${file}}")
          set(${file} "${SB_APPLICATION_CONFIG_DIR}/${${file}}")
        else()
          # Relative paths are relative to 'west topdir'.
          #
          # This is the only file that has a relative check to topdir likely
          # from the historical callouts to "west" itself before using
          # imgtool. So, this is maintained here for backward compatibility
          #
          if(NOT WEST OR NOT WEST_TOPDIR)
            message(FATAL_ERROR "Can't sign images for MCUboot: west workspace "
                                "undefined. To fix, ensure `west topdir` is a "
                                "valid workspace directory.")
          endif()
          set(${file} "${WEST_TOPDIR}/${${file}}")
        endif()
      endif()

      if(NOT EXISTS "${${file}}")
        message(FATAL_ERROR "Can't sign images for MCUboot: can't find file "
                            "${${file}} (Note: Relative paths are searched "
                            "through SB_APPLICATION_CONFIG_DIR="
                            "\"${SB_APPLICATION_CONFIG_DIR}\" and WEST_TOPDIR="
                            "\"${WEST_TOPDIR}\")")
      endif()
    endforeach()
  endif()

  # No imgtool, no signed binaries.
  if(NOT DEFINED IMGTOOL)
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool. "
                        "To fix, install imgtool with pip3, or add the mcuboot "
                        "repository to the west manifest and ensure it has "
                        "a scripts/imgtool.py file.")
    return()
  endif()

  # No hex2bin, no signed bin files.
  if(SB_CONFIG_BUILD_OUTPUT_BIN AND NOT DEFINED HEX2BIN)
    message(FATAL_ERROR "Can't convert HEX files for MCUboot: can't find "
                        "hex2bin. To fix, install hex2bin with pip3.")
    return()
  endif()

  # This script uses only HEX files as input.
  if(NOT SB_CONFIG_BUILD_OUTPUT_HEX)
      message(FATAL_ERROR "Can't sign merged images for MCUboot: The "
                          "SB_CONFIG_BUILD_OUTPUT_HEX is not enabled,"
                          "so there's nothing to sign.")
      return()
  endif()

  # Fetch extra arguments to imgtool from the main image Kconfig.
  set(CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS)
  sysbuild_get(CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS KCONFIG)
  if(NOT CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS STREQUAL "")
    separate_arguments(imgtool_args UNIX_COMMAND
      ${CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS})
  else()
    set(imgtool_args)
  endif()

  # Fetch VID and CID values from the main image Kconfig.
  set(CONFIG_MCUBOOT_IMGTOOL_UUID_VID)
  set(CONFIG_MCUBOOT_IMGTOOL_UUID_CID)
  set(CONFIG_MCUBOOT_IMGTOOL_UUID_VID_NAME)
  set(CONFIG_MCUBOOT_IMGTOOL_UUID_CID_NAME)
  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_UUID_VID IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_IMGTOOL_UUID_VID KCONFIG)
  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_UUID_CID IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_IMGTOOL_UUID_CID KCONFIG)
  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_UUID_VID_NAME IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_IMGTOOL_UUID_VID_NAME KCONFIG)
  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_UUID_CID_NAME IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_IMGTOOL_UUID_CID_NAME KCONFIG)

  if(CONFIG_MCUBOOT_IMGTOOL_UUID_VID)
    set(imgtool_args ${imgtool_args} --vid
      "${CONFIG_MCUBOOT_IMGTOOL_UUID_VID_NAME}")
  endif()

  if(CONFIG_MCUBOOT_IMGTOOL_UUID_CID)
    set(imgtool_args ${imgtool_args} --cid
      "${CONFIG_MCUBOOT_IMGTOOL_UUID_CID_NAME}")
  endif()

  # Fetch version and flags from the main image Kconfig.
  set(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION)
  set(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE)
  set(CONFIG_NCS_IS_VARIANT_IMAGE)
  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG)
  sysbuild_get(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE KCONFIG)
  sysbuild_get(CONFIG_NCS_IS_VARIANT_IMAGE IMAGE ${main_image} VAR
    CONFIG_NCS_IS_VARIANT_IMAGE CACHE)

  # Fetch devicetree details for flash and slot information.
  dt_chosen(flash_node TARGET mcuboot PROPERTY "zephyr,flash")
  dt_prop(write_block_size TARGET mcuboot PATH "${flash_node}" PROPERTY
    "write-block-size")
  dt_nodelabel(slot0_path TARGET mcuboot NODELABEL "slot0_partition" REQUIRED)
  dt_partition_addr(slot0_addr PATH "${slot0_path}" TARGET mcuboot REQUIRED)
  dt_reg_size(slot0_size TARGET mcuboot PATH ${slot0_path})
  dt_nodelabel(slot1_path TARGET mcuboot NODELABEL "slot1_partition" REQUIRED)
  dt_partition_addr(slot1_addr PATH "${slot1_path}" TARGET mcuboot REQUIRED)
  dt_reg_size(slot1_size TARGET mcuboot PATH ${slot1_path})
  if(NOT write_block_size)
    set(write_block_size 4)
    message(WARNING "slot0_partition write block size devicetree parameter is "
                    "missing, assuming write block size is 4")
  endif()

  # Fetch devicetree details for the active code partition.
  dt_chosen(code_flash TARGET ${main_image} PROPERTY "zephyr,code-partition")
  dt_partition_addr(code_addr PATH "${code_flash}" TARGET ${main_image} REQUIRED)
  set(start_offset)
  sysbuild_get(start_offset IMAGE ${main_image} VAR CONFIG_ROM_START_OFFSET
    KCONFIG)

  # Append key file path.
  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_args --key "${keyfile}" ${imgtool_args})
  endif()

  # Construct imgtool command, based on the selected MCUboot mode.
  set(imgtool_rom_command)
  if(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT OR
     SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP)
    if(CONFIG_NCS_IS_VARIANT_IMAGE)
      set(slot_size ${slot1_size})
    else()
      set(slot_size ${slot0_size})
    endif()
    set(imgtool_rom_command --rom-fixed ${code_addr})
  else()
    message(FATAL_ERROR "Only Direct XIP MCUboot modes are supported.")
    return()
  endif()

  # Basic 'imgtool sign' command with known image information.
  set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign
      --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --header-size
      ${start_offset} --slot-size ${slot_size} ${imgtool_rom_command})
  set(imgtool_args --align ${write_block_size} ${imgtool_args})

  # Extensionless prefix of any output file.
  sysbuild_get(BINARY_DIR IMAGE ${main_image} VAR APPLICATION_BINARY_DIR CACHE)
  sysbuild_get(BINARY_BIN_FILE IMAGE ${main_image} VAR
    CONFIG_KERNEL_BIN_NAME KCONFIG)
  cmake_path(GET BINARY_DIR PARENT_PATH sysbuild_build_dir)
  if(CONFIG_NCS_IS_VARIANT_IMAGE)
    cmake_path(APPEND sysbuild_build_dir "zephyr"
      "${BINARY_BIN_FILE}_secondary_app" OUTPUT_VARIABLE output)
  else()
    cmake_path(APPEND sysbuild_build_dir "zephyr" "${BINARY_BIN_FILE}"
      OUTPUT_VARIABLE output)
  endif()

  # List of additional build byproducts.
  set(byproducts ${output}.merged.hex)

  sysbuild_get(CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512 IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512 KCONFIG)
  sysbuild_get(CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE IMAGE ${main_image}
    VAR CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE KCONFIG)

  # Set proper hash calculation algorithm for signing
  if(CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE)
    set(imgtool_args --pure ${imgtool_args})
  elseif(CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512)
    set(imgtool_args --sha 512 ${imgtool_args})
  endif()

  if(NOT "${keyfile_enc}" STREQUAL "")
    if(SB_CONFIG_BOOT_ENCRYPTION_ALG_AES_256)
      set(imgtool_args ${imgtool_args} --encrypt-keylen 256)
    endif()
  endif()

  # Set up .hex outputs.
  if(SB_CONFIG_BUILD_OUTPUT_HEX)
    list(APPEND byproducts ${output}.signed.hex)
    set(final_artifact_hex ${output}.signed.hex)
    set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${output}.signed.hex"
        CACHE FILEPATH "Signed kernel hex file" FORCE
    )

    if(NOT "${keyfile_enc}" STREQUAL "")
      # When encryption is enabled, set the encrypted bit when signing the image
      # but do not encrypt the data, this means that when the image is moved out
      # of the primary into the secondary, it will be encrypted rather than
      # being in unencrypted.
      list(APPEND imgtool_cmd COMMAND
        ${imgtool_sign} ${imgtool_args} --encrypt "${keyfile_enc}" --clear
        ${merged_hex} ${output}.signed.hex)
    else()
      list(APPEND imgtool_cmd COMMAND
        ${imgtool_sign} ${imgtool_args} ${merged_hex} ${output}.signed.hex)
    endif()

    if(CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE OR
       SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT)
      list(APPEND byproducts ${output}.signed.confirmed.hex)
      set(final_artifact_hex ${output}.signed.confirmed.hex)
      set(BYPRODUCT_KERNEL_SIGNED_CONFIRMED_HEX_NAME
        "${output}.signed.confirmed.hex" CACHE FILEPATH
        "Signed and confirmed kernel hex file" FORCE)
      list(APPEND imgtool_cmd COMMAND
        ${imgtool_sign} ${imgtool_args} --pad --confirm ${merged_hex}
        ${output}.signed.confirmed.hex)
    endif()

    if(NOT "${keyfile_enc}" STREQUAL "")
      list(APPEND byproducts ${output}.signed.encrypted.hex)
      set(BYPRODUCT_KERNEL_SIGNED_ENCRYPTED_HEX_NAME
        "${output}.signed.encrypted.hex" CACHE FILEPATH
        "Signed and encrypted kernel hex file" FORCE)
      list(APPEND imgtool_cmd COMMAND
        ${imgtool_sign} ${imgtool_args} --encrypt "${keyfile_enc}" ${merged_hex}
        ${output}.signed.encrypted.hex)
    endif()

  list(APPEND imgtool_cmd COMMAND
      ${CMAKE_COMMAND} -E copy ${final_artifact_hex} ${output}.merged.hex)
  endif()

  # Set up .bin outputs.
  if(SB_CONFIG_BUILD_OUTPUT_BIN)
    set(bin_byproducts)
    foreach(hex_file ${byproducts})
      string(REGEX REPLACE "\\.[^.]*$" "" file_path ${hex_file})
      list(APPEND imgtool_cmd COMMAND
        ${PYTHON_EXECUTABLE} ${HEX2BIN} ${hex_file} ${file_path}.bin
      )
      list(APPEND bin_byproducts ${file_path}.bin)
    endforeach()
    list(APPEND byproducts ${bin_byproducts})
  endif()

  add_custom_target(
    signed_${main_image}
    ALL
    ${imgtool_cmd}
    DEPENDS
    ${merged_hex}
    BYPRODUCTS
    ${byproducts}
    COMMAND_EXPAND_LISTS
    COMMENT "Create MCUboot artifacts"
  )
endfunction()
