# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)
include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/ironside_se_tlv.cmake)

# Find the slot address and size for the given image.
# Usage:
#   find_slot_addr_size(<image_name> <slot_abs_addr> <slot_addr> <slot_size> <alt_slot_addr>
#                       <is_variant>)
#
# Parameters:
#   image_name: Name of the image to find the slot address and size for.
#   slot_addr: Output variable to store the slot address.
#   slot_size: Output variable to store the slot size.
#   slot_abs_addr: Output variable to store the slot absolute address.
#   alt_slot_abs_addr: Output variable to store the alternate slot address.
#   is_variant: Output variable to store whether the image is a variant image.
function(find_slot_addr_size image_name slot_addr slot_size slot_abs_addr alt_slot_abs_addr is_variant)
  # If the mcuboot image is in the list of images, use it as the slotinfo image.
  if(mcuboot IN_LIST IMAGES)
    set(slotinfo_image mcuboot)
  else()
    set(slotinfo_image ${image_name})
  endif()

  dt_chosen(code_flash TARGET ${image_name} PROPERTY "zephyr,code-partition")
  dt_partition_addr(code_addr PATH "${code_flash}" TARGET ${image_name} ABSOLUTE REQUIRED)
  dt_partition_size(code_size PATH "${code_flash}" TARGET ${image_name})
  math(EXPR code_end_addr "${code_addr} + ${code_size}" OUTPUT_FORMAT HEXADECIMAL)

  # Iterate over the updateable images to find the slot address and size.
  MATH(EXPR updateable_images_index_max
    "${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES} + ${SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES} - 1"
  )
  message(DEBUG "[${slotinfo_image}] Looking for slot info for image ${image_name} "
    "code_addr: ${code_addr}, code_end_addr: ${code_end_addr}"
  )

  foreach(image_index RANGE 0 ${updateable_images_index_max})
    math(EXPR slot_0 "${image_index} * 2")
    if(NOT SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
      math(EXPR slot_1 "${image_index} * 2 + 1")
    endif()

    dt_nodelabel(slot0_path TARGET ${slotinfo_image} NODELABEL "slot${slot_0}_partition" REQUIRED)
    dt_partition_addr(slot0_addr PATH "${slot0_path}" TARGET ${slotinfo_image} REQUIRED)
    dt_partition_addr(slot0_abs_addr PATH "${slot0_path}" TARGET ${slotinfo_image} ABSOLUTE
      REQUIRED)
    dt_partition_size(slot0_size PATH "${slot0_path}" TARGET ${slotinfo_image})
    math(EXPR slot0_abs_end_addr "${slot0_abs_addr} + ${slot0_size}" OUTPUT_FORMAT HEXADECIMAL)

    if(NOT SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
      dt_nodelabel(slot1_path TARGET ${slotinfo_image} NODELABEL "slot${slot_1}_partition" REQUIRED)
      dt_partition_addr(slot1_addr PATH "${slot1_path}" TARGET ${slotinfo_image} REQUIRED)
      dt_partition_addr(slot1_abs_addr PATH "${slot1_path}" TARGET ${slotinfo_image} ABSOLUTE
        REQUIRED)
      dt_partition_size(slot1_size PATH "${slot1_path}" TARGET ${slotinfo_image})
      math(EXPR slot1_ebs_end_addr "${slot1_abs_addr} + ${slot1_size}" OUTPUT_FORMAT HEXADECIMAL)
    endif()

    message(DEBUG "  Checking slot${slot_0}_abs_addr: ${slot0_abs_addr}, "
      "slot${slot_0}_abs_end_addr: ${slot0_abs_end_addr}"
    )

    # Check if zephyr,code-partition is located within the slot<2 * i>_partition.
    # If so, deduce that the image is located inside the primary slot of image <i>.
    if(${code_addr} GREATER_EQUAL ${slot0_abs_addr} AND ${code_end_addr} LESS_EQUAL ${slot0_abs_end_addr})
      set(${slot_addr} ${slot0_addr} PARENT_SCOPE)
      set(${slot_abs_addr} ${slot0_abs_addr} PARENT_SCOPE)
      set(${slot_size} ${slot0_size} PARENT_SCOPE)
      set(${is_variant} FALSE PARENT_SCOPE)

      # Return an absolute address of the alternative slot, so an update .hex file can be generated.
      if(NOT SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
        set(${alt_slot_abs_addr} ${slot1_abs_addr} PARENT_SCOPE)
      else()
        set(${alt_slot_abs_addr} ${slot0_abs_addr} PARENT_SCOPE)
      endif()
      return()
    endif()

    # Check if zephyr,code-partition is located within the slot<2 * i + 1>_partition.
    # If so, deduce that the image is located inside the secondary slot of image <i>.
    # All variant images uses the secondary slots.
    if(NOT SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
      message(DEBUG "  Checking slot${slot_1}_abs_addr: ${slot1_abs_addr}, "
        "slot${slot_1}_ebs_end_addr: ${slot1_ebs_end_addr}"
      )

      if(${code_addr} GREATER_EQUAL ${slot1_abs_addr} AND ${code_end_addr} LESS_EQUAL ${slot1_ebs_end_addr})
        set(${slot_addr} ${slot1_addr} PARENT_SCOPE)
        set(${slot_abs_addr} ${slot1_abs_addr} PARENT_SCOPE)
        set(${slot_size} ${slot1_size} PARENT_SCOPE)
        set(${is_variant} TRUE PARENT_SCOPE)

        # Return an absolute address of the alternative slot, so an update .hex file can be generated.
        set(${alt_slot_abs_addr} ${slot0_abs_addr} PARENT_SCOPE)
        return()
      endif()
    endif()
  endforeach()

  message(FATAL_ERROR "Failed to find slot address for ${image_name}")
endfunction()

# Pop a modifier from the list of modifiers.
# Usage:
#   pop_modifier(<modifiers> <modifier_name> <modifier_args> <remaining_modifiers>)
#
# Parameters:
#   modifiers: List of modifiers to pop from.
#   modifier_name: Output variable to store the additional output suffix for the modifier.
#   modifier_args: Output variable to store the additional imgtool args for the modifier.
#   remaining_modifiers: Output variable to store the remaining modifiers.
function(pop_modifier modifiers modifier_name modifier_args remaining_modifiers)
  list(POP_FRONT modifiers pop_name pop_arg)

  # Use "<null>" if either arguments or the suffix should not be added.
  # This is used because CMake does not support empty strings as elements in lists.
  if("${pop_name}" STREQUAL "<null>")
    set(pop_name "")
  endif()

  if("${pop_arg}" STREQUAL "<null>")
    set(pop_arg "")
  endif()

  set(${modifier_name} ${pop_name} PARENT_SCOPE)
  set(${modifier_args} ${pop_arg} PARENT_SCOPE)
  set(${remaining_modifiers} ${modifiers} PARENT_SCOPE)
endfunction()

# Sign a file with a list of modifiers.
# Usage:
#   sign_with_modifiers(<modifiers> <imgtool_sign> <file_to_sign> <output> <imgtool_cmds>
#                       <byproducts>)
#
# Parameters:
#   modifiers: List of modifiers to apply while signing the file.
#   imgtool_sign: The base imgtool sign command.
#   file_to_sign: The file to sign.
#   output: The output signed file name.
#   imgtool_cmds: Output variable to store the list of imgtool commands.
#   byproducts: Output variable to store the list of byproducts, generated by the imgtool commands.
function(sign_with_modifiers modifiers imgtool_sign file_to_sign output imgtool_cmds byproducts)
  list(LENGTH modifiers modifiers_length)

  if(modifiers_length GREATER_EQUAL 4)
    # If there are at least 4 modifiers, fork the signing process into two branches.
    # As a result, the number of output files will be doubled.
    pop_modifier("${modifiers}" modifier_0_name modifier_0_args modifiers)
    pop_modifier("${modifiers}" modifier_1_name modifier_1_args modifiers)

    sign_with_modifiers("${modifiers}" "${imgtool_sign} ${modifier_0_args}" ${file_to_sign}
      ${output}${modifier_0_name} imgtool_cmds_a byproducts_a)
    sign_with_modifiers("${modifiers}" "${imgtool_sign} ${modifier_1_args}" ${file_to_sign}
      ${output}${modifier_1_name} imgtool_cmds_b byproducts_b)

    # Merge the commands and byproducts from the two branches.
    list(APPEND imgtool_cmds_all ${imgtool_cmds_a} ${imgtool_cmds_b})
    list(APPEND byproducts_all ${byproducts_a} ${byproducts_b})
  elseif(modifiers_length GREATER_EQUAL 2)
    # If there are the last two modifiers left, it means that the final set of arguments is being
    # processed.
    # Pop the last modifiers and call the signing function once again, so even if the list is empty,
    # the signing logic follows the same execution path.
    pop_modifier("${modifiers}" modifier_0_name modifier_0_args modifiers)
    sign_with_modifiers("${modifiers}" "${imgtool_sign} ${modifier_0_args}" ${file_to_sign}
      ${output}${modifier_0_name} imgtool_cmds_all byproducts_all)
  else()
    # No usable modifiers left. Prepare the signing command.
    # At this stage, a single command and output file are generated.
    list(APPEND byproducts_all ${output})
    separate_arguments(imgtool_args UNIX_COMMAND "${imgtool_sign}")
    list(APPEND imgtool_cmds_all COMMAND ${imgtool_args} ${file_to_sign} ${output})
  endif()

  # Propagate back the commands and byproducts.
  set(${byproducts} ${byproducts_all} PARENT_SCOPE)
  set(${imgtool_cmds} ${imgtool_cmds_all} PARENT_SCOPE)
endfunction()

# Sign the given image with MCUboot.
# Usage:
#   mcuboot_sign_sysbuild(<main_image>)
#
# Parameters:
#   main_image: Name of the main image to sign.
#
# This function is responsible for signing the main image using MCUboot format.
# It will generate the signed artifacts for the main image.
# The signed artifacts are generated by the imgtool sign command.
function(mcuboot_sign_sysbuild main_image)
  set(imgtool_cmd)
  set(imgtool_depends)

  ###
  # Check if all required tools are available.
  ###

  find_program(IMGTOOL imgtool.py HINTS ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/ NAMES imgtool
    NAMES_PER_DIR)
  # No imgtool, no signed binaries.
  if("${IMGTOOL}" STREQUAL "IMGTOOL-NOTFOUND")
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool. "
                        "To fix, install imgtool with pip3, or add the mcuboot "
                        "repository to the west manifest and ensure it has "
                        "a scripts/imgtool.py file.")
    return()
  endif()

  ###
  # Handle custom (extra) imgtool arguments.
  ###

  # Fetch extra arguments to imgtool from the main image Kconfig.
  sysbuild_get(extra_imgtool_args IMAGE ${main_image} VAR CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS KCONFIG)
  if(NOT extra_imgtool_args STREQUAL "")
    separate_arguments(imgtool_args UNIX_COMMAND ${extra_imgtool_args})
  else()
    set(imgtool_args)
  endif()

  ###
  # Handle (signing, encryption) keys.
  ###

  sysbuild_get(keyfile IMAGE ${main_image} VAR CONFIG_MCUBOOT_SIGNATURE_KEY_FILE KCONFIG)
  sysbuild_get(keyfile_enc IMAGE ${main_image} VAR CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE KCONFIG)
  sysbuild_get(generate_unsigned_image IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE KCONFIG)
  string(CONFIGURE "${keyfile}" keyfile)
  string(CONFIGURE "${keyfile_enc}" keyfile_enc)

  if(NOT "${generate_unsigned_image}")
    # Check for misconfiguration.
    if("${keyfile}" STREQUAL "")
      # No signature key file, no signed binaries. No error, though:
      # this is the documented behavior.
      message(WARNING "Neither CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE or "
                      "CONFIG_MCUBOOT_SIGNATURE_KEY_FILE are set for image ${image}. "
                      "The generated build will not be bootable by MCUboot unless it is signed "
                      "manually/externally.")
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

  # Append the signing key file path.
  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_args --key "${keyfile}" ${imgtool_args})
  endif()

  sysbuild_get(signature_type_pure IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE KCONFIG)
  sysbuild_get(bootloader_uses_sha512 IMAGE ${main_image} VAR CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512
    KCONFIG)

  # Set proper hash calculation algorithm for signing
  if(signature_type_pure)
    set(imgtool_args --pure ${imgtool_args})
  elseif(bootloader_uses_sha512)
    set(imgtool_args --sha 512 ${imgtool_args})
  endif()

  if(NOT "${keyfile_enc}" STREQUAL "")
    if(SB_CONFIG_BOOT_ENCRYPTION_ALG_AES_256)
      set(imgtool_args ${imgtool_args} --encrypt-keylen 256)
    endif()

    # Signature type determines key exchange scheme; ED25519 here means
    # ECIES-X25519 is used. Default to HMAC-SHA512 for ECIES-X25519.
    # Only .encrypted.bin file gets the ENCX25519/ENCX25519_SHA512, the
    # just signed one does not.
    # Only NRF54L gets the HMAC-SHA512, other remain with previously used
    # SHA256.
    sysbuild_get(signature_type_ed25519 IMAGE ${main_image} VAR
      CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_ED25519 KCONFIG)
    if(SB_CONFIG_SOC_SERIES_NRF54L AND signature_type_ed25519)
      set(imgtool_args ${imgtool_args} --hmac-sha 512)
    endif()
  endif()

  ###
  # Fetch basic information from the DTS.
  ###

  find_slot_addr_size(${main_image} slot_addr slot_size slot_abs_addr alt_slot_abs_addr is_variant_image)

  ###
  # Handle downgrade prevention.
  ###

  # Fetch the downgrade prevention params from the main image Kconfig.
  sysbuild_get(hardware_downgrade_prevention IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION KCONFIG)
  sysbuild_get(hardware_downgrade_prevention_counter_value IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE KCONFIG)
  if(hardware_downgrade_prevention)
    set(imgtool_args ${imgtool_args} --security-counter
      ${hardware_downgrade_prevention_counter_value})
  endif()

  ###
  # Handle UUID VID and CID values.
  ###

  # Fetch VID and CID values from the main image Kconfig.
  sysbuild_get(imgtool_uuid_vid IMAGE ${main_image} VAR CONFIG_MCUBOOT_IMGTOOL_UUID_VID KCONFIG)
  sysbuild_get(imgtool_uuid_cid IMAGE ${main_image} VAR CONFIG_MCUBOOT_IMGTOOL_UUID_CID KCONFIG)
  sysbuild_get(imgtool_uuid_vid_name IMAGE ${main_image} VAR CONFIG_MCUBOOT_IMGTOOL_UUID_VID_NAME KCONFIG)
  sysbuild_get(imgtool_uuid_cid_name IMAGE ${main_image} VAR CONFIG_MCUBOOT_IMGTOOL_UUID_CID_NAME KCONFIG)
  if(imgtool_uuid_vid)
    set(imgtool_args ${imgtool_args} --vid "\"${imgtool_uuid_vid_name}\"")
  endif()

  if(imgtool_uuid_cid)
    set(imgtool_args ${imgtool_args} --cid "\"${imgtool_uuid_cid_name}\"")
  endif()

  ###
  # Handle DTS-based bootloader configuration.
  ###

  # Check if there is a bootloader configuration in the main image devicetree.
  dt_comp_path(mcuboot_configs COMPATIBLE "nordic,mcuboot" TARGET ${main_image})
  if(mcuboot_configs)
    sysbuild_get(binary_dir IMAGE ${main_image} VAR APPLICATION_BINARY_DIR CACHE)
    cmake_path(APPEND binary_dir "zephyr" "edt.pickle" OUTPUT_VARIABLE edt_pickle)
    message(STATUS "Passing DTS-based MCUboot configuration: ${mcuboot_configs}")
    list(APPEND imgtool_args  --edt-config "${edt_pickle}")
  endif()

  ###
  # Handle IronSide SE TLVs.
  ###

  if(SB_CONFIG_NCS_MCUBOOT_LOAD_PERIPHCONF)
    set(tlv_extra_imgtool_args)
    set(tlv_extra_sign_depends)
    generate_ironside_se_tlvs_sysbuild(${main_image} tlv_extra_imgtool_args tlv_extra_sign_depends)
    list(APPEND imgtool_args ${tlv_extra_imgtool_args})
    list(APPEND imgtool_depends ${tlv_extra_sign_depends})
  endif()

  ###
  # Handle MCUboot manifest file.
  ###

  sysbuild_get(imgtool_append_manifest IMAGE ${main_image} VAR
    CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST KCONFIG)
  if(imgtool_append_manifest)
    sysbuild_get(binary_dir IMAGE ${main_image} VAR APPLICATION_BINARY_DIR CACHE)
    cmake_path(GET binary_dir PARENT_PATH sysbuild_build_dir)
    sysbuild_get(app_config_dir IMAGE ${main_image} VAR APPLICATION_CONFIG_DIR CACHE)

    if(is_variant_image)
      cmake_path(APPEND app_config_dir "mcuboot_manifest_secondary.yaml" OUTPUT_VARIABLE manifest_path)

      if(NOT EXISTS "${manifest_path}")
        cmake_path(APPEND sysbuild_build_dir "mcuboot_manifest_secondary.yaml" OUTPUT_VARIABLE
          manifest_path)
      endif()

      # Since parsing manifest requires output, signed artifacts from listed images to be present,
      # add dependencies between the manifest image and other updateable and signed images.
      UpdateableImage_Get(images GROUP "VARIANT")

      foreach(image ${images})
        list(APPEND imgtool_depends ${image}_signed)
      endforeach()
    else()
      cmake_path(APPEND app_config_dir "mcuboot_manifest.yaml" OUTPUT_VARIABLE manifest_path)

      if(NOT EXISTS "${manifest_path}")
        cmake_path(APPEND sysbuild_build_dir "mcuboot_manifest.yaml" OUTPUT_VARIABLE manifest_path)
      endif()

      # Since parsing manifest requires output, signed artifacts from listed images to be present,
      # add dependencies between the manifest image and other updateable and signed images.
      UpdateableImage_Get(images GROUP "DEFAULT")

      foreach(image ${images})
        list(APPEND imgtool_depends ${image}_signed)
      endforeach()
    endif()

    set(imgtool_args ${imgtool_args} --manifest "${manifest_path}")
  endif()

  ###
  # Handle slot (offset, write block size), MCUboot mode and version information.
  ###

  # If MCUboot image is present in the build, use its DTS as a source for slot
  # information, otherwise use the main image DTS. This allows for signing
  # images even if the MCUboot image is not built as part of the application.
  if(mcuboot IN_LIST IMAGES)
    set(slotinfo_image mcuboot)
  else()
    set(slotinfo_image ${main_image})
  endif()

  # Fetch devicetree details for flash and slot information.
  dt_chosen(flash_node TARGET ${slotinfo_image} PROPERTY "zephyr,flash")
  dt_prop(write_block_size TARGET ${slotinfo_image} PATH "${flash_node}" PROPERTY
    "write-block-size")

  if(NOT write_block_size)
    set(write_block_size 4)
    message(WARNING "slot0_partition write block size devicetree parameter is "
                    "missing, assuming write block size is 4")
  endif()

  # Fetch devicetree details for executable RAM region.
  dt_chosen(ram_load_dev TARGET ${slotinfo_image} PROPERTY "mcuboot,ram-load-dev")

  if(DEFINED ram_load_dev)
    dt_reg_addr(ram_load_address TARGET ${slotinfo_image} PATH ${ram_load_dev})
  else()
    dt_chosen(chosen_ram TARGET ${slotinfo_image} PROPERTY "zephyr,sram")
    dt_reg_addr(ram_load_address TARGET ${slotinfo_image} PATH ${chosen_ram})
  endif()

  # Fetch devicetree details for the active code partition.
  dt_chosen(code_flash TARGET ${main_image} PROPERTY "zephyr,code-partition")
  dt_partition_addr(code_addr PATH "${code_flash}" TARGET ${main_image} REQUIRED)
  sysbuild_get(start_offset IMAGE ${main_image} VAR CONFIG_ROM_START_OFFSET KCONFIG)

  # Construct imgtool command, based on the selected MCUboot mode.
  math(EXPR start_offset_dts "${code_addr} - ${slot_addr}")
  set(imgtool_rom_command)
  set(pad_header)

  if(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT OR
     SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP)
    set(imgtool_rom_command --rom-fixed ${slot_addr})
  elseif(SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
  elseif(SB_CONFIG_MCUBOOT_MODE_SINGLE_APP_RAM_LOAD)
    set(start_offset_dts 0)
    # RAM load requires setting the location of where to load the image to
    set(imgtool_rom_command --load-addr ${ram_load_address})
    set(write_block_size 1)
  elseif(SB_CONFIG_MCUBOOT_MODE_RAM_LOAD)
    set(start_offset_dts 0)
    # RAM load requires setting the location of where to load the image to
    set(imgtool_rom_command --load-addr ${ram_load_address})
    set(write_block_size 1)
  elseif(SB_CONFIG_MCUBOOT_MODE_RAM_LOAD_WITH_REVERT)
    set(start_offset_dts 0)
    # RAM load requires setting the location of where to load the image to
    set(imgtool_rom_command --load-addr ${ram_load_address})
  elseif(SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER)
    set(imgtool_rom_command --rom-fixed ${slot_addr})
  else()
    # This case handles modes that use slot1 as staging area:
    # - All MCUBOOT_MODE_SWAP_* modes
    # - MCUBOOT_MODE_OVERWRITE_ONLY
  endif()

  if(SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER OR SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY)
    set(imgtool_args --overwrite-only --align 1 ${imgtool_args})
  else()
    set(imgtool_args --align ${write_block_size} ${imgtool_args})
  endif()

  # Adjust start offset, based on the active slot and code partition address.
  if(start_offset_dts GREATER 0)
    math(EXPR start_offset "${start_offset} + ${start_offset_dts}")
    set(pad_header "--pad-header")
  endif()

  sysbuild_get(build_with_tfm IMAGE ${main_image} VAR CONFIG_BUILD_WITH_TFM KCONFIG)
  if(build_with_tfm)
    # For TF-M builds the MCUboot header lives at the start of the combined slot
    # (slot0_s_partition). Use TFM_MCUBOOT_HEADER_SIZE so the correct 0x200 value
    # is used even when ROM_START_OFFSET is 0 for TF-M NS builds.
    sysbuild_get(tfm_mcuboot_header_size IMAGE ${main_image} VAR CONFIG_TFM_MCUBOOT_HEADER_SIZE
      KCONFIG)
    set(start_offset ${tfm_mcuboot_header_size})
    # TF-M combined images need --pad-header because the MCUboot header gap is
    # at the combined slot start (in tfm_s.hex), not at the NS partition start.
    set(pad_header "--pad-header")
  endif()

  # Fetch version and flags from the main image Kconfig.
  sysbuild_get(imgtool_sign_version IMAGE ${main_image} VAR CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION
    KCONFIG)

  # Basic 'imgtool sign' command with known image information.
  set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version
    ${imgtool_sign_version} --header-size ${start_offset} --slot-size ${slot_size}
    ${pad_header} ${imgtool_rom_command})

  ###
  # Handle signing command modifiers.
  ###

  # Construct the signing command modifiers list.
  if(SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
    set(imgtool_sign ${imgtool_sign} --hex-addr ${slot_abs_addr})
  else()
    if(SB_CONFIG_MCUBOOT_MODE_RAM_LOAD OR SB_CONFIG_MCUBOOT_MODE_RAM_LOAD_WITH_REVERT)
      list(APPEND modifiers
        "<null>" "--hex-addr ${slot_abs_addr}"
        ".slot1" "--hex-addr ${alt_slot_abs_addr}"
      )
    endif()
  endif()
  if(NOT "${keyfile_enc}" STREQUAL "")
    list(APPEND modifiers
      "<null>" "--encrypt ${keyfile_enc} --clear"
      ".encrypted" "--encrypt ${keyfile_enc}"
    )
  endif()

  sysbuild_get(generate_confirmed_image IMAGE ${main_image} VAR
    CONFIG_MCUBOOT_GENERATE_CONFIRMED_IMAGE KCONFIG)
  if(generate_confirmed_image OR SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT)
    list(APPEND modifiers
      "<null>" "<null>"
      ".confirmed" "--pad --confirm"
    )
  endif()

  ###
  # Generate signing commands.
  ###

  set(imgtool_cmds)
  set(byproducts)
  list(JOIN imgtool_sign " " imgtool_sign_str)

  # The existing signing logic generates artifacts following the pattern:
  #   <binary_file>.signed.<encrypted/confirmed/modified>.<ext>
  # Append the "signed" suffix before the suffixes, appended by modifiers.
  sysbuild_get(kernel_bin_file IMAGE ${main_image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
  sysbuild_get(binary_dir IMAGE ${main_image} VAR APPLICATION_BINARY_DIR CACHE)
  cmake_path(APPEND binary_dir "zephyr" "${kernel_bin_file}.signed" OUTPUT_VARIABLE output)

  # Set up .hex outputs.
  if(SB_CONFIG_BUILD_OUTPUT_HEX)
    list(JOIN imgtool_args " " imgtool_args_str)
    set(modifiers_hex ${modifiers})
    list(APPEND modifiers_hex
      ".hex" "${imgtool_args_str}"
    )

    sysbuild_get(file_to_sign_hex IMAGE ${main_image} VAR RUNNERS_HEX_FILE_TO_SIGN CACHE)
    cmake_path(APPEND binary_dir "zephyr" ${file_to_sign_hex} OUTPUT_VARIABLE file_to_sign_hex)
    if("${keyfile}" STREQUAL "")
      # If there is no signing key - simply copy input binary to the output file.
      list(APPEND imgtool_cmds_hex COMMAND ${CMAKE_COMMAND} -E copy "${file_to_sign_hex}"
        "${output}.hex")
      list(APPEND byproducts_hex "${output}.hex")
    else()
      sign_with_modifiers("${modifiers_hex}" "${imgtool_sign_str}" ${file_to_sign_hex} ${output}
        imgtool_cmds_hex byproducts_hex)
    endif()
    list(APPEND imgtool_cmds ${imgtool_cmds_hex})
    list(APPEND byproducts ${byproducts_hex})
  endif()

  # Set up .bin outputs.
  if(SB_CONFIG_BUILD_OUTPUT_BIN)
    ###
    # Handle compressed image support (available only for .bin artifacts).
    ###

    sysbuild_get(compressed_image_support_enabled IMAGE ${main_image} VAR
      CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT_ENABLED KCONFIG)

    if(compressed_image_support_enabled)
      sysbuild_get(lzma_max_dict_size IMAGE ${main_image} VAR CONFIG_NRF_COMPRESS_LZMA_MAX_DICT_SIZE
        KCONFIG)
      sysbuild_get(lzma_pb IMAGE ${main_image} VAR CONFIG_NRF_COMPRESS_LZMA_PB KCONFIG)
      sysbuild_get(lzma_lc IMAGE ${main_image} VAR CONFIG_NRF_COMPRESS_LZMA_LC KCONFIG)
      sysbuild_get(lzma_lp IMAGE ${main_image} VAR CONFIG_NRF_COMPRESS_LZMA_LP KCONFIG)
      sysbuild_get(lzma_compression_preset IMAGE ${main_image} VAR CONFIG_NRF_COMPRESS_LZMA_COMPRESSION_PRESET KCONFIG)
      set(imgtool_args ${imgtool_args} --compression lzma2armthumb)
      set(imgtool_args ${imgtool_args} --compression-lzma-dictsize=${lzma_max_dict_size})
      set(imgtool_args ${imgtool_args} --compression-lzma-pb=${lzma_pb})
      set(imgtool_args ${imgtool_args} --compression-lzma-lc=${lzma_lc})
      set(imgtool_args ${imgtool_args} --compression-lzma-lp=${lzma_lp})
      set(imgtool_args ${imgtool_args} --compression-lzma-preset=${lzma_compression_preset})
    endif()

    list(JOIN imgtool_args " " imgtool_args_str)
    set(modifiers_bin ${modifiers})
    list(APPEND modifiers_bin
      ".bin" "${imgtool_args_str}"
    )

    sysbuild_get(file_to_sign_bin IMAGE ${main_image} VAR RUNNERS_BIN_FILE_TO_SIGN CACHE)
    cmake_path(APPEND binary_dir "zephyr" ${file_to_sign_bin} OUTPUT_VARIABLE file_to_sign_bin)
    if("${keyfile}" STREQUAL "")
      # If there is no signing key - simply copy input binary to the output file.
      list(APPEND imgtool_cmds_bin COMMAND ${CMAKE_COMMAND} -E copy "${file_to_sign_bin}"
        "${output}.bin")
      list(APPEND byproducts_bin "${output}.bin")
    else()
      sign_with_modifiers("${modifiers_bin}" "${imgtool_sign_str}" ${file_to_sign_bin} ${output}
        imgtool_cmds_bin byproducts_bin)
    endif()
    list(APPEND imgtool_cmds ${imgtool_cmds_bin})
    list(APPEND byproducts ${byproducts_bin})
  endif()

  ###
  # Generate signing targets.
  ###

  add_custom_target(
    ${main_image}_signed
    ALL
    ${imgtool_cmds}
    DEPENDS
    ${main_image}_extra_byproducts
    ${slotinfo_image}
    ${imgtool_depends}
    BYPRODUCTS
    ${byproducts}
    COMMAND_EXPAND_LISTS
    COMMENT "Create MCUboot artifacts"
  )

  ###
  # Generate unified signed artifacts.
  #
  # .hex file: used for flashing by the image flasher.
  # .bin file: used for generating the .hex update artifact.
  ###

  list(POP_FRONT byproducts_hex final_artifact_hex)

  if(final_artifact_hex)
    message(STATUS "Configuring image flasher for ${final_artifact_hex}")
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/${main_image}.signed.hex
      COMMAND
      ${CMAKE_COMMAND} -E copy ${final_artifact_hex} "${CMAKE_BINARY_DIR}/${main_image}.signed.hex"
      DEPENDS ${main_image}_signed
      COMMAND_EXPAND_LISTS
      COMMENT "Create final signed.hex artifact for flashing"
    )
  endif()

  list(POP_FRONT byproducts_bin final_artifact_bin)

  if(final_artifact_bin)
    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/${main_image}.update.signed.hex
      COMMAND ${PYTHON_EXECUTABLE} -c
        "import sys; import intelhex; intelhex.bin2hex(sys.argv[1], sys.argv[2], int(sys.argv[3], 16)) "
        ${final_artifact_bin}
        ${CMAKE_BINARY_DIR}/${main_image}.update.signed.hex
        ${alt_slot_abs_addr}
      DEPENDS ${main_image}_signed
      COMMENT "Create final update.signed.hex artifact for flashing"
      VERBATIM
    )
  endif()
endfunction()

###
# Execute signing logic for all updateable and signed images.
###

if(SB_CONFIG_MCUBOOT_SYSBUILD_SIGN)
  UpdateableImage_Get(images ALL)

  foreach(image ${images})
    # Images signed through sysbuild are marked as build only.
    # The flashing is handled by an additional image_flasher target,
    # automatically generated for each updateble and signed image.
    set_target_properties(${image} PROPERTIES BUILD_ONLY true)
    mcuboot_sign_sysbuild(${image})
  endforeach()
endif()
