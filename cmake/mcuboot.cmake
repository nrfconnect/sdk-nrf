# Included by mcuboot/zephyr/CMakeLists.txt
set(MCUBOOT_DIR ${ZEPHYR_BASE}/../bootloader/mcuboot)

if(CONFIG_BOOTLOADER_MCUBOOT)
  function(sign to_sign_hex output_prefix offset sign_depends signed_hex_out)
    set(op ${output_prefix})
    set(signed_hex ${op}_signed.hex)
    set(${signed_hex_out} ${signed_hex} PARENT_SCOPE)
    set(to_sign_bin ${op}_to_sign.bin)
    set(update_bin ${op}_update.bin)
    set(moved_test_update_hex ${op}_moved_test_update.hex)
    set(test_update_hex ${op}_test_update.hex)

    add_custom_command(
      OUTPUT
      ${update_bin}            # Signed binary of input hex.
      ${signed_hex}            # Signed hex of input hex.
      ${test_update_hex}       # Signed hex with IMAGE_MAGIC
      ${moved_test_update_hex} # Signed hex with IMAGE_MAGIC located at secondary slot

      COMMAND
      # Create signed hex file from input hex file.
      # This does not have the IMAGE_MAGIC at the end. So for this hex file
      # to be applied by mcuboot, the application is required to write the
      # IMAGE_MAGIC into the image trailer.
      ${sign_cmd}
      ${to_sign_hex}
      ${signed_hex}

      COMMAND
      # Create binary version of the input hex file, this is done so that we
      # can create a signed binary file which will be transferred in OTA
      # updates.
      ${CMAKE_OBJCOPY}
      --input-target=ihex
      --output-target=binary
      ${to_sign_hex}
      ${to_sign_bin}

      COMMAND
      # Sign the binary version of the input hex file.
      ${sign_cmd}
      ${to_sign_bin}
      ${update_bin}

      COMMAND
      # Create signed hex file from input hex file *with* IMAGE_MAGIC.
      # As this includes the IMAGE_MAGIC in its image trailer, it will be
      # swapped in by mcuboot without any invocation from the app. Note,
      # however, that this this hex file is located in the same address space
      # as the input hex file, so in order for it to work as a test update,
      # it needs to be moved.
      ${sign_cmd}
      --pad # Adds IMAGE_MAGIC to end of slot.
      ${to_sign_hex}
      ${test_update_hex}

      COMMAND
      # Create version of test update which is located at the secondary slot.
      # Hence, if a programmer is given this hex file, it will flash it
      # to the secondary slot, and upon reboot mcuboot will swap in the
      # contents of the hex file.
      ${CMAKE_OBJCOPY}
      --input-target=ihex
      --output-target=ihex
      --change-address ${offset}
      ${test_update_hex}
      ${moved_test_update_hex}

      DEPENDS
      ${sign_depends}
      )
  endfunction()

  if (CONFIG_BUILD_S1_VARIANT AND ("${CONFIG_S1_VARIANT_IMAGE_NAME}" STREQUAL "mcuboot"))
    # Inject this configuration from parent image to mcuboot.
    set(conf_path "${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/build_s1.conf")
    string(FIND ${mcuboot_OVERLAY_CONFIG} ${conf_path} out)
    if (${out} EQUAL -1)
      set(mcuboot_OVERLAY_CONFIG
        "${mcuboot_OVERLAY_CONFIG} ${conf_path}"
        CACHE STRING "" FORCE)
    endif()
  endif()

  add_child_image(mcuboot ${MCUBOOT_DIR}/boot/zephyr)

  set(merged_hex_file
    ${PROJECT_BINARY_DIR}/mcuboot_primary_app.hex)
  set(merged_hex_file_depends
    mcuboot_primary_app_hex$<SEMICOLON>${PROJECT_BINARY_DIR}/mcuboot_primary_app.hex)
  set(sign_merged
    $<TARGET_EXISTS:partition_manager>)
  set(app_to_sign_hex
    $<IF:${sign_merged},${merged_hex_file},${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}>)
  set(app_sign_depends
    $<IF:${sign_merged},${merged_hex_file_depends},zephyr_final>)

  set(sign_cmd
    ${PYTHON_EXECUTABLE}
    ${MCUBOOT_DIR}/scripts/imgtool.py
    sign
    --key ${MCUBOOT_DIR}/${CONFIG_BOOT_SIGNATURE_KEY_FILE}
    --header-size $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PAD_SIZE>
    --align       ${CONFIG_DT_FLASH_WRITE_BLOCK_SIZE}
    --version     ${CONFIG_MCUBOOT_IMAGE_VERSION}
    --slot-size   $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_SIZE>
    --pad-header
    )

  set(app_offset $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_SIZE>)

  sign(${app_to_sign_hex}     # Hex to sign
    ${PROJECT_BINARY_DIR}/app # Prefix for generated files
    ${app_offset}             # Offset
    ${app_sign_depends}       # Dependencies
    app_signed_hex            # Generated hex output variable
    )

  add_custom_target(mcuboot_sign_target DEPENDS ${app_signed_hex})

  set_property(GLOBAL PROPERTY
    mcuboot_primary_app_PM_HEX_FILE
    ${app_signed_hex}
    )
  set_property(GLOBAL PROPERTY
    mcuboot_primary_app_PM_TARGET
    mcuboot_sign_target
    )

  if (CONFIG_BUILD_S1_VARIANT AND ("${CONFIG_S1_VARIANT_IMAGE_NAME}" STREQUAL "mcuboot"))
    # Secure Boot (B0) is enabled, and we have to build update candidates
    # for both S1 and S0.

    # We need to override some attributes of the parent slot S0/S1.
    # Which contains both the S0/S1 image and the padding/header.
    foreach(parent_slot s0;s1)
      set(slot ${parent_slot}_image)

      # Fetch the target and hex file for the current slot.
      # Note that these hex files are already signed by B0.
      get_property(${slot}_target GLOBAL PROPERTY ${slot}_PM_TARGET)
      get_property(${slot}_hex GLOBAL PROPERTY ${slot}_PM_HEX_FILE)

      # The gap from S0/S1 partition is calculated by partition manager
      # and stored in its target.
      set(slot_offset
        $<TARGET_PROPERTY:partition_manager,${parent_slot}_TO_SECONDARY>)

      # Depend on both the target for the hex file, and the hex file itself.
      set(dependencies "${${slot}_target};${${slot}_hex}")

      set(out_path ${PROJECT_BINARY_DIR}/signed_by_mcuboot_and_b0_${slot})

      sign(${${slot}_hex} # Hex file to sign
        ${out_path}
        ${slot_offset}
        "${dependencies}" # Need "..." to make it a list.
        signed_hex        # Created file variable
        )

      # We now have to override the S0/S1 partition, so use `parent_slot`
      # variable, which is "s0" and "s1" respectively. This to get partition
      # manager to override the implicitly assigned container hex files.

      # Wrapper target for the generated hex file.
      add_custom_target(signed_${parent_slot}_target DEPENDS ${signed_hex})

      # Override the container hex file.
      set_property(GLOBAL PROPERTY
        ${parent_slot}_PM_HEX_FILE
        ${signed_hex}
        )

      # Override the container hex file target.
      set_property(GLOBAL PROPERTY
        ${parent_slot}_PM_TARGET
        signed_${parent_slot}_target
        )
    endforeach()
  endif()
endif()

