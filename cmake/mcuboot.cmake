# Included by mcuboot/zephyr/CMakeLists.txt
set(MCUBOOT_DIR ${ZEPHYR_BASE}/../bootloader/mcuboot)

if(CONFIG_BOOTLOADER_MCUBOOT)

  include(${ZEPHYR_BASE}/../nrf/cmake/fw_zip.cmake)

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

      # Add Zigbee OTA header to signed application
      COMMAND
      ${zb_add_ota_header_cmd}

      DEPENDS
      ${sign_depends}
      )
  endfunction()

  if (CONFIG_BUILD_S1_VARIANT AND ("${CONFIG_S1_VARIANT_IMAGE_NAME}" STREQUAL "mcuboot"))
    # Inject this configuration from parent image to mcuboot.
    add_overlay_config(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/build_s1.conf
      )
  endif()

  add_child_image(
    NAME mcuboot
    SOURCE_DIR ${MCUBOOT_DIR}/boot/zephyr
    )

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

  if (DEFINED mcuboot_CONF_FILE)
    get_filename_component(mcuboot_CONF_DIR ${mcuboot_CONF_FILE} DIRECTORY)
    if (EXISTS ${mcuboot_CONF_DIR}/${CONFIG_BOOT_SIGNATURE_KEY_FILE})
      set(mcuboot_key_file ${mcuboot_CONF_DIR}/${CONFIG_BOOT_SIGNATURE_KEY_FILE})
    endif()
  endif()

  # Set default key
  if (NOT DEFINED mcuboot_key_file)
    message(WARNING "
      ---------------------------------------------------------
      --- WARNING: Using default MCUBoot key, it should not ---
      --- be used for production.                           ---
      ---------------------------------------------------------
      \n"
    )
    set(mcuboot_key_file ${MCUBOOT_DIR}/${CONFIG_BOOT_SIGNATURE_KEY_FILE})
  endif()

  set(sign_cmd
    ${PYTHON_EXECUTABLE}
    ${MCUBOOT_DIR}/scripts/imgtool.py
    sign
    --key ${mcuboot_key_file}
    --header-size $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PAD_SIZE>
    --align       ${CONFIG_MCUBOOT_FLASH_WRITE_BLOCK_SIZE}
    --version     ${CONFIG_MCUBOOT_IMAGE_VERSION}
    --slot-size   $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_SIZE>
    --pad-header
    )

  if(CONFIG_ZIGBEE)
    set(zb_add_ota_header_cmd
      ${PYTHON_EXECUTABLE}
      ${NRF_DIR}/scripts/bootloader/zb_add_ota_header.py
      --application ${PROJECT_BINARY_DIR}/app_update.bin
      --application-version-string ${CONFIG_MCUBOOT_IMAGE_VERSION}
      --zigbee-manufacturer-id ${CONFIG_ZIGBEE_OTA_MANUFACTURER_ID}
      --zigbee-image-type ${CONFIG_ZIGBEE_OTA_IMAGE_TYPE}
      --zigbee-comment ${CONFIG_ZIGBEE_OTA_COMMENT}
      --zigbee-ota-min-hw-version ${CONFIG_ZIGBEE_OTA_MIN_HW_VERSION}
      --zigbee-ota-max-hw-version ${CONFIG_ZIGBEE_OTA_MAX_HW_VERSION}
      --out-directory ${PROJECT_BINARY_DIR}
      )
    else()
      set(zb_add_ota_header_cmd "")
    endif(CONFIG_ZIGBEE)

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

  generate_dfu_zip(
    TARGET mcuboot_sign_target
    OUTPUT ${PROJECT_BINARY_DIR}/dfu_application.zip
    BIN_FILES ${PROJECT_BINARY_DIR}/app_update.bin
    TYPE application
    SCRIPT_PARAMS
    "load_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
    "version_MCUBOOT=${CONFIG_MCUBOOT_IMAGE_VERSION}"
    )

  if (CONFIG_BT_RPMSG_NRF53 AND CONFIG_SOC_NRF5340_CPUAPP)
    # The bootloader on the network core is enabled. The validation of this
    # bootloader is performed by MCUBoot on the application core. Hence we
    # need a target for creating the signed binary of the network core
    # application.

    include(${CMAKE_BINARY_DIR}/hci_rpmsg/shared_vars.cmake)

    # TODO replace with proper domain once PR is in
    sign(${nrf5340pdk_nrf5340_cpunet_PM_APP_HEX}
      ${PROJECT_BINARY_DIR}/net_core_app
      $<TARGET_PROPERTY:partition_manager,net_app_TO_SECONDARY>
      hci_rpmsg_subimage
      net_core_app_signed_hex
      )

    add_custom_target(
      net_core_app_sign_target
      ALL
      DEPENDS ${net_core_app_signed_hex}
      )

  endif()

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

    # Generate zip file with both update candidates
    set(s0_name signed_by_mcuboot_and_b0_s0_image_update.bin)
    set(s0_bin_path ${PROJECT_BINARY_DIR}/${s0_name})
    set(s1_name signed_by_mcuboot_and_b0_s1_image_update.bin)
    set(s1_bin_path ${PROJECT_BINARY_DIR}/${s1_name})

    # Create dependency to ensure explicit build order. This is needed to have
    # a single target represent the state when both s0 and s1 imags are built.
    add_dependencies(
      signed_s1_target
      signed_s0_target
      )

    generate_dfu_zip(
      TARGET signed_s1_target
      OUTPUT ${PROJECT_BINARY_DIR}/dfu_mcuboot.zip
      BIN_FILES ${s0_bin_path} ${s1_bin_path}
      TYPE mcuboot
      SCRIPT_PARAMS
      "${s0_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>"
      "${s1_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>"
      "version_MCUBOOT=${CONFIG_MCUBOOT_IMAGE_VERSION}"
      "version_B0=${CONFIG_FW_INFO_FIRMWARE_VERSION}"
      )
  endif()
endif()

