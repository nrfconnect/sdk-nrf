add_subdirectory(${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/zephyr
                 ${CMAKE_CURRENT_BINARY_DIR}/boot/bootutil/zephyr
)

if (CONFIG_MCUBOOT)
  if (CONFIG_NRF53_MULTI_IMAGE_UPDATE)
    set_shared(IMAGE ${IMAGE_NAME} PROPERTY NRF53_MULTI_IMAGE_UPDATE
      ${CONFIG_NRF53_MULTI_IMAGE_UPDATE})
  endif()

  if (CONFIG_NRF53_RECOVERY_NETWORK_CORE)
    set_shared(IMAGE ${IMAGE_NAME} PROPERTY NRF53_RECOVERY_NETWORK_CORE
      ${CONFIG_NRF53_RECOVERY_NETWORK_CORE})
  endif()

  dt_nodelabel(ram_flash_ctrl_dev NODELABEL nordic_ram_flash_controller)
  if (DEFINED ram_flash_ctrl_dev)
    # Share the information about the ram_flash node with the parent image.
    # This information is used to create the partition manager region.
    dt_nodelabel(ram_flash_dev NODELABEL flash_sim0)
    dt_reg_addr(ram_flash_addr PATH ${ram_flash_dev})
    dt_reg_size(ram_flash_size PATH ${ram_flash_dev})
    set_shared(IMAGE ${IMAGE_NAME} PROPERTY RAM_FLASH_ADDR ${ram_flash_addr})
    set_shared(IMAGE ${IMAGE_NAME} PROPERTY RAM_FLASH_SIZE ${ram_flash_size})
    set_shared(IMAGE ${IMAGE_NAME} PROPERTY RAM_FLASH_LABEL nordic_ram_flash_controller)
  endif()
endif()

if(CONFIG_BOOTLOADER_MCUBOOT)
  if (CONFIG_NCS_IS_VARIANT_IMAGE)
    # When building the variant of an image, the configuration of the variant image
    # is identical to the base image, except 'CONFIG_NCS_IS_VARIANT_IMAGE'. The
    # logic below should only be performed once, for the base image, not for the variant variant.
    # Hence, we have this early return to avoid execution of this file for the variant image.
    return()
  endif()

  include(${ZEPHYR_BASE}/../nrf/cmake/fw_zip.cmake)
  include(${ZEPHYR_BASE}/../nrf/cmake/dfu_multi_image.cmake)

  function(sign)
    # Signs a hex image
    #
    # Required arguments are:
    # SIGNED_BIN_FILE_IN - (required) Hex image to sign
    # SIGNED_HEX_FILE_NAME_PREFIX - (required) Prefix of the output image
    # SLOT_SIZE - (required) The size of the primary partition
    # SIGNED_HEX_FILE_OUT - (required) Signed hex output image
    #
    # Optional arguments:
    # START_ADDRESS_OFFSET - (optional) Offset
    # SIGNED_BIN_FILE_OUT - (optional) Signed bin output image
    # SIGNED_HEX_TEST_FILE_OUT - (optional) Signed hex output test image including IMAGE_MAGIC
    # IMAGE_DEPENDENCIES - (optional) Dependencies between signed image and other images
    # ROM_FIXED - (optional) Start address of image
    # DEPENDS - (optional) One or more dependencies for signing the hex image
    set(oneValueArgs SIGNED_BIN_FILE_IN SIGNED_HEX_FILE_NAME_PREFIX SLOT_SIZE SIGNED_HEX_FILE_OUT)
    set(oneValueOptionalArgs SIGNED_BIN_FILE_OUT SIGNED_HEX_TEST_FILE_OUT START_ADDRESS_OFFSET IMAGE_DEPENDENCIES ROM_FIXED)
    set(multiValueArgs DEPENDS)
    cmake_parse_arguments(SIGN_ARG "" "${oneValueArgs};${oneValueOptionalArgs}" "${multiValueArgs}" ${ARGN})
    check_arguments_required_all(sign SIGN_ARG ${oneValueArgs})

    set(signed_hex ${SIGN_ARG_SIGNED_HEX_FILE_NAME_PREFIX}_signed.hex)
    set(${SIGN_ARG_SIGNED_HEX_FILE_OUT} ${signed_hex} PARENT_SCOPE)
    set(to_sign_bin ${SIGN_ARG_SIGNED_HEX_FILE_NAME_PREFIX}_to_sign.bin)
    set(update_bin ${SIGN_ARG_SIGNED_HEX_FILE_NAME_PREFIX}_update.bin)
    set(test_update_hex ${SIGN_ARG_SIGNED_HEX_FILE_NAME_PREFIX}_test_update.hex)
    if(DEFINED SIGN_ARG_SIGNED_BIN_FILE_OUT)
      set(${SIGN_ARG_SIGNED_BIN_FILE_OUT} ${update_bin} PARENT_SCOPE)
    endif()
    if(DEFINED SIGN_ARG_SIGNED_HEX_TEST_FILE_OUT)
      set(${SIGN_ARG_SIGNED_HEX_TEST_FILE_OUT} ${test_update_hex} PARENT_SCOPE)
    endif()

    if(DEFINED SIGN_ARG_ROM_FIXED)
      set(SIGN_ARG_ROM_FIXED --rom-fixed ${SIGN_ARG_ROM_FIXED})
    endif()

    # Ensure that it is possible to move the secondary slot to the memory
    # mapped area for external flash if the secondary slot is located in
    # external flash. Only devices with 'qspi' should have hex files generated
    # for the secondary slot, since these are the only ones with a memory mapped
    # area.
    dt_nodelabel(qspi_node NODELABEL "qspi")
    if ((NOT CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY OR DEFINED qspi_node) AND
        SIGN_ARG_START_ADDRESS_OFFSET)
      set(moved_test_update_hex
        ${SIGN_ARG_SIGNED_HEX_FILE_NAME_PREFIX}_moved_test_update.hex
        )

      if(DEFINED SIGN_ARG_SIGNED_HEX_TEST_FILE_OUT)
        set(${SIGN_ARG_SIGNED_HEX_TEST_FILE_OUT}
          ${test_update_hex} ${moved_test_update_hex} PARENT_SCOPE
          )
      endif()

      add_custom_command(
        OUTPUT
        ${moved_test_update_hex} # Signed hex with IMAGE_MAGIC located at secondary slot

        COMMAND
        # Create version of test update which is located at the secondary slot.
        # Hence, if a programmer is given this hex file, it will flash it
        # to the secondary slot, and upon reboot mcuboot will swap in the
        # contents of the hex file.
        ${CMAKE_OBJCOPY}
        --input-target=ihex
        --output-target=ihex
        --change-address=${SIGN_ARG_START_ADDRESS_OFFSET}
        --gap-fill=0xff
        ${test_update_hex}
        ${moved_test_update_hex}

        DEPENDS
        ${SIGN_ARG_DEPENDS}
        ${test_update_hex}
        )
    endif()

    if(DEFINED SIGN_ARG_IMAGE_DEPENDENCIES)
      set(sign_dependencies "-d;\"${SIGN_ARG_IMAGE_DEPENDENCIES}\"")
    endif()

    add_custom_command(
      OUTPUT
      ${signed_hex}            # Signed hex of input hex.

      COMMAND
      # Create signed hex file from input hex file.
      # This does not have the IMAGE_MAGIC at the end. So for this hex file
      # to be applied by mcuboot, the application is required to write the
      # IMAGE_MAGIC into the image trailer.
      ${sign_cmd}
      ${sign_dependencies}
      ${SIGN_ARG_ROM_FIXED}
      --slot-size ${SIGN_ARG_SLOT_SIZE}
      ${SIGN_ARG_SIGNED_BIN_FILE_IN}
      ${signed_hex}

      DEPENDS
      ${SIGN_ARG_DEPENDS}
        )

    add_custom_command(
      OUTPUT
      ${update_bin}            # Signed binary of input hex.

      COMMAND
      # Create binary version of the input hex file, this is done so that we
      # can create a signed binary file which will be transferred in OTA
      # updates.
      ${CMAKE_OBJCOPY}
      --input-target=ihex
      --output-target=binary
      --gap-fill=0xff
      ${SIGN_ARG_SIGNED_BIN_FILE_IN}
      ${to_sign_bin}

      COMMAND
      # Sign the binary version of the input hex file.
      ${sign_cmd}
      ${sign_dependencies}
      ${SIGN_ARG_ROM_FIXED}
      --slot-size ${SIGN_ARG_SLOT_SIZE}
      ${to_sign_bin}
      ${update_bin}
      DEPENDS
      ${SIGN_ARG_DEPENDS}
        )

    add_custom_command(
      OUTPUT
      ${test_update_hex}       # Signed hex with IMAGE_MAGIC

      COMMAND
      # Create signed hex file from input hex file *with* IMAGE_MAGIC.
      # As this includes the IMAGE_MAGIC in its image trailer, it will be
      # swapped in by mcuboot without any invocation from the app. Note,
      # however, that this this hex file is located in the same address space
      # as the input hex file, so in order for it to work as a test update,
      # it needs to be moved.
      ${sign_cmd}
      ${sign_dependencies}
      ${SIGN_ARG_ROM_FIXED}
      --slot-size ${SIGN_ARG_SLOT_SIZE}
      --pad # Adds IMAGE_MAGIC to end of slot.
      ${SIGN_ARG_SIGNED_BIN_FILE_IN}
      ${test_update_hex}

      DEPENDS
      ${SIGN_ARG_DEPENDS}
      ${update_bin}
      )
  endfunction()

  if (CONFIG_BUILD_S1_VARIANT AND ("${CONFIG_S1_VARIANT_IMAGE_NAME}" STREQUAL "mcuboot"))
    # Inject this configuration from parent image to mcuboot.
    add_overlay_config(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/build_s1.conf
      )
  endif()

  if (CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY)
    # By passing this value we don't require the user to pass any overlay files
    # to the MCUboot child image for storing the secondary partition in
    # external flash.
    add_overlay_config(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/partition_manager/ext_flash_mcuboot_secondary.conf
      )
  endif()

  if (CONFIG_BUILD_WITH_TFM)
    # Configure MCUBoot to be able to boot TFM
    add_overlay_config(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/tfm.conf
      )

    # Add fault injection protection to MCUBOOT
    add_overlay_config(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/fih_low_enable.conf
    )
  endif()

  if (CONFIG_NRF53_UPGRADE_NETWORK_CORE AND CONFIG_ADD_MCUBOOT_MEDIATE_SIM_FLASH_DTS)
    # To allow multi image update of the network core we add a devicetree
    # overlay which defines a flash controller used for emulating flash
    # in RAM.
    add_overlay_dts(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/flash_sim.overlay
      )
  endif()

  # For devices with USB then a USB CDC node in mcuboot is required.
  # As USB CDC is generally needed for this mcuboot use-case it should be
  # considered if this solution could be better generalized.
  if (CONFIG_MCUBOOT_USB_SUPPORT)
    add_overlay_dts(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/usb.overlay
      )
  endif()

  # Enable the same option in the MCUBoot child image
  if (CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    add_overlay_config(
      mcuboot
      ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/hw_counters.conf
    )

    set(imgtool_security_counter --security-counter ${CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE})
  endif()

  add_child_image(
    NAME mcuboot
    SOURCE_DIR ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr
    )

  if (CONFIG_BUILD_S1_VARIANT)
    add_child_image(
      NAME s1_image
      PRELOAD_IMAGE mcuboot
      )
  elseif(CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT)
    add_child_image(
      NAME mcuboot_secondary_app
      PRELOAD_IMAGE app
      )
  endif()

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

  if (NOT DEFINED CONFIG_BOOT_SIGNATURE_KEY_FILE)
    get_shared(mcuboot_sign_key IMAGE mcuboot PROPERTY SIGNATURE_KEY_FILE)
    set(CONFIG_BOOT_SIGNATURE_KEY_FILE ${mcuboot_sign_key})
  endif ()

  foreach (filepath ${mcuboot_CONF_FILE})
    file(STRINGS ${filepath} mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE
         REGEX "^CONFIG_BOOT_SIGNATURE_KEY_FILE=")
    if (mcuboot_CONFIG_BOOT_SIGNATURE_KEY_FILE)
      get_filename_component(mcuboot_CONF_DIR ${filepath} DIRECTORY)
    endif()
  endforeach()

  if(IS_ABSOLUTE ${CONFIG_BOOT_SIGNATURE_KEY_FILE})
    set(mcuboot_key_file ${CONFIG_BOOT_SIGNATURE_KEY_FILE})
  elseif (DEFINED mcuboot_CONF_DIR)
    if (EXISTS ${mcuboot_CONF_DIR}/${CONFIG_BOOT_SIGNATURE_KEY_FILE})
      set(mcuboot_key_file ${mcuboot_CONF_DIR}/${CONFIG_BOOT_SIGNATURE_KEY_FILE})
    endif()
  endif()

  if(CONFIG_SIGN_IMAGES)
    # Set default key
    if (NOT DEFINED mcuboot_key_file)
      message(WARNING "
        ---------------------------------------------------------
        --- WARNING: Using default MCUBoot key, it should not ---
        --- be used for production.                           ---
        ---------------------------------------------------------
        \n"
      )
      set(mcuboot_key_file ${ZEPHYR_MCUBOOT_MODULE_DIR}/${CONFIG_BOOT_SIGNATURE_KEY_FILE})
    endif()

    set(app_core_binary_name app_update.bin)

    execute_process(COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/imgtool.py
      getpub -k ${mcuboot_key_file}
      OUTPUT_QUIET
      ERROR_QUIET
      RESULT_VARIABLE ret_val
    )

    if(${ret_val} EQUAL 2)
      message(WARNING "Key file `${mcuboot_key_file}` does not contain a valid \
                       private key. Signing of images will be disabled.")
      message("Disable signing with `CONFIG_SIGN_IMAGES=n` to silence this warning.")
      return()
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

    set(sign_cmd
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/imgtool.py
      sign
      --key ${mcuboot_key_file}
      --header-size $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PAD_SIZE>
      --align       ${CONFIG_MCUBOOT_FLASH_WRITE_BLOCK_SIZE}
      --version     ${CONFIG_MCUBOOT_IMAGE_VERSION}
      --pad-header
      ${imgtool_security_counter}
      ${imgtool_extra}
      )

    set(app_offset $<TARGET_PROPERTY:partition_manager,app_TO_SECONDARY>)

    if (CONFIG_NRF53_ENFORCE_IMAGE_VERSION_EQUALITY)
      # Specify image pair IDs to generate dependencies between images.
      set(APPLICATION_IMAGE_PAIR_IDX 0)
      set(NETWORK_IMAGE_PAIR_IDX 1)

      # Add dependency inside application core image to require the latest network core image.
      set(sign_dependencies IMAGE_DEPENDENCIES "(${NETWORK_IMAGE_PAIR_IDX},${CONFIG_MCUBOOT_IMAGE_VERSION})")
    endif()

    if (CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT)
      set(rom_fixed ROM_FIXED $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_ADDRESS>)
    else()
      set(rom_fixed)
    endif()

    sign(
      SIGNED_BIN_FILE_IN ${app_to_sign_hex}
      SIGNED_HEX_FILE_NAME_PREFIX ${PROJECT_BINARY_DIR}/app
      SLOT_SIZE $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_SIZE>
      START_ADDRESS_OFFSET ${app_offset}
      SIGNED_HEX_FILE_OUT app_signed_hex
      SIGNED_BIN_FILE_OUT app_signed_bin
      SIGNED_HEX_TEST_FILE_OUT app_signed_test_hex
      ${sign_dependencies}
      ${rom_fixed}
      DEPENDS ${app_sign_depends}
      )

    add_custom_target(mcuboot_sign_target
      DEPENDS ${app_signed_hex} ${app_signed_bin} ${app_signed_test_hex}
      )

    set_property(GLOBAL PROPERTY
      mcuboot_primary_app_PM_HEX_FILE
      ${app_signed_hex}
      )
    set_property(GLOBAL PROPERTY
      mcuboot_primary_app_PM_TARGET
      mcuboot_sign_target
      )

    if (CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT)
      get_shared(mcuboot_secondary_app_bin_dir IMAGE mcuboot_secondary_app PROPERTY ZEPHYR_BINARY_DIR)
      set(mcuboot_secondary_app_hex ${mcuboot_secondary_app_bin_dir}/zephyr.hex)

      set(rom_fixed ROM_FIXED $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_SECONDARY_ADDRESS>)

      sign(
        SIGNED_BIN_FILE_IN ${mcuboot_secondary_app_hex}
        SIGNED_HEX_FILE_NAME_PREFIX ${PROJECT_BINARY_DIR}/mcuboot_secondary_app
        SLOT_SIZE $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_SIZE>
        SIGNED_HEX_FILE_OUT mcuboot_secondary_app_signed_hex
        SIGNED_BIN_FILE_OUT mcuboot_secondary_app_signed_bin
        SIGNED_HEX_TEST_FILE_OUT mcuboot_secondary_app_signed_test_hex
        ${rom_fixed}
        DEPENDS mcuboot_secondary_app_subimage ${mcuboot_secondary_app_bin_dir}/zephyr.hex
        )

      add_custom_target(mcuboot_secondary_app_sign_target
        DEPENDS
        ${mcuboot_secondary_app_signed_hex}
        ${mcuboot_secondary_app_signed_bin}
        ${mcuboot_secondary_app_signed_test_hex}
        )

      set_property(GLOBAL PROPERTY
        mcuboot_secondary_app_PM_HEX_FILE
        ${mcuboot_secondary_app_signed_hex}
        )

      set_property(GLOBAL PROPERTY
        mcuboot_secondary_app_PM_TARGET
        mcuboot_secondary_app_sign_target
        )
    endif()

    get_shared(cpunet_signed_app_hex IMAGE CPUNET PROPERTY PM_SIGNED_APP_HEX)

    if (CONFIG_NRF53_UPGRADE_NETWORK_CORE
        AND DEFINED cpunet_signed_app_hex)
      # Network core application updates are enabled.
      # We know this since MCUBoot is enabled on the application core, and
      # a network core child image is included in the build.
      # These updates are verified by the application core MCUBoot.
      # Create a signed variant of the network core application.

      get_shared(cpunet_images IMAGE CPUNET PROPERTY PM_DOMAIN_IMAGES)
      foreach(image ${cpunet_images})
        if(${image} MATCHES "CPUNET:(.*)")
          set(image_name ${CMAKE_MATCH_1})
          if(TARGET ${image_name}_subimage)
            get_shared(${image_name}_byproducts IMAGE ${image_name} PROPERTY BUILD_BYPRODUCTS)
            list(APPEND sign_depends ${image_name}_subimage ${${image_name}_byproducts})
          endif()
        endif()
      endforeach()

      set(net_core_binary_name net_core_app_update.bin)

      if (CONFIG_NRF53_ENFORCE_IMAGE_VERSION_EQUALITY)
        # Add dependency inside network core image to require the latest application core image.
        set(sign_dependencies IMAGE_DEPENDENCIES "(${APPLICATION_IMAGE_PAIR_IDX},${CONFIG_MCUBOOT_IMAGE_VERSION})")
      endif()

      sign(
        SIGNED_BIN_FILE_IN ${cpunet_signed_app_hex}
        SIGNED_HEX_FILE_NAME_PREFIX ${PROJECT_BINARY_DIR}/net_core_app
        SLOT_SIZE $<TARGET_PROPERTY:partition_manager,net_app_slot_size>
        START_ADDRESS_OFFSET $<TARGET_PROPERTY:partition_manager,net_app_TO_SECONDARY>
        SIGNED_HEX_FILE_OUT net_core_app_signed_hex
        SIGNED_BIN_FILE_OUT net_core_app_signed_bin
        SIGNED_HEX_TEST_FILE_OUT net_core_app_signed_test_hex
        ${sign_dependencies}
        DEPENDS ${sign_depends}
        )

      add_custom_target(
        net_core_app_sign_target
        DEPENDS ${net_core_app_signed_hex}
          ${net_core_app_signed_bin}
          ${net_core_app_signed_test_hex}
        )

      add_dependencies(
        mcuboot_sign_target
        net_core_app_sign_target
        )

      get_shared(net_core_soc IMAGE net_core PROPERTY SOC)
      get_shared(net_core_version IMAGE net_core PROPERTY VERSION)

      set(generate_bin_files
        ${PROJECT_BINARY_DIR}/${app_core_binary_name}
        ${PROJECT_BINARY_DIR}/${net_core_binary_name}
        )
      set(generate_script_params
        "${app_core_binary_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
        "${app_core_binary_name}image_index=0"
        "${app_core_binary_name}slot_index_primary=1"
        "${app_core_binary_name}slot_index_secondary=2"
        "${app_core_binary_name}version_MCUBOOT=${CONFIG_MCUBOOT_IMAGE_VERSION}"
        "${net_core_binary_name}image_index=1"
        "${net_core_binary_name}slot_index_primary=3"
        "${net_core_binary_name}slot_index_secondary=4"
        "${net_core_binary_name}load_address=$<TARGET_PROPERTY:partition_manager,CPUNET_PM_APP_ADDRESS>"
        "${net_core_binary_name}board=${CONFIG_DOMAIN_CPUNET_BOARD}"
        "${net_core_binary_name}version=${net_core_version}"
        "${net_core_binary_name}soc=${net_core_soc}"
        )
    else()
      # No network core update
      if(CONFIG_BOOT_BUILD_DIRECT_XIP_VARIANT)
        set(secondary_app_core_binary_name mcuboot_secondary_app_update.bin)
        set(generate_bin_files
          ${PROJECT_BINARY_DIR}/${app_core_binary_name}
          ${PROJECT_BINARY_DIR}/${secondary_app_core_binary_name}
          )
        set(generate_script_params
          "${app_core_binary_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_APP_ADDRESS>"
          "${app_core_binary_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMAGE_VERSION}"
          "${app_core_binary_name}slot=0"
          "${secondary_app_core_binary_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_SECONDARY_APP_ADDRESS>"
          "${secondary_app_core_binary_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMAGE_VERSION}"
          "${secondary_app_core_binary_name}slot=1"
          )
      else()
        set(generate_bin_files
          ${PROJECT_BINARY_DIR}/${app_core_binary_name}
          )
        set(generate_script_params
          "load_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
          "version_MCUBOOT=${CONFIG_MCUBOOT_IMAGE_VERSION}"
          )
      endif()
    endif()

    generate_dfu_zip(
      OUTPUT ${PROJECT_BINARY_DIR}/dfu_application.zip
      BIN_FILES ${generate_bin_files}
      TYPE application
      SCRIPT_PARAMS ${generate_script_params}
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

        set(out_path ${PROJECT_BINARY_DIR}/signed_by_mcuboot_and_b0_${slot})

        sign(
          SIGNED_BIN_FILE_IN ${${slot}_hex}
          SIGNED_HEX_FILE_NAME_PREFIX ${out_path}
          SLOT_SIZE $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_SIZE>
          START_ADDRESS_OFFSET ${slot_offset}
          SIGNED_HEX_FILE_OUT signed_hex
          SIGNED_BIN_FILE_OUT signed_bin
          SIGNED_HEX_TEST_FILE_OUT signed_test_hex
          DEPENDS ${${slot}_target} ${${slot}_hex}
          )

        # We now have to override the S0/S1 partition, so use `parent_slot`
        # variable, which is "s0" and "s1" respectively. This to get partition
        # manager to override the implicitly assigned container hex files.

        # Wrapper target for the generated hex file.
        add_custom_target(signed_${parent_slot}_target
          DEPENDS ${signed_hex} ${signed_bin} ${signed_test_hex}
          )

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
  endif(CONFIG_SIGN_IMAGES)
endif()

if (CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD)
  set(dfu_multi_image_ids)
  set(dfu_multi_image_paths)
  set(dfu_multi_image_targets)

  if (CONFIG_DFU_MULTI_IMAGE_PACKAGE_APP)
    list(APPEND dfu_multi_image_ids 0)
    list(APPEND dfu_multi_image_paths "${PROJECT_BINARY_DIR}/${app_core_binary_name}")
    list(APPEND dfu_multi_image_targets mcuboot_sign_target)
  endif()

  if (CONFIG_DFU_MULTI_IMAGE_PACKAGE_NET)
    list(APPEND dfu_multi_image_ids 1)
    list(APPEND dfu_multi_image_paths "${PROJECT_BINARY_DIR}/${net_core_binary_name}")
    list(APPEND dfu_multi_image_targets net_core_app_sign_target)
  endif()

  if (CONFIG_DFU_MULTI_IMAGE_PACKAGE_MCUBOOT)
    list(APPEND dfu_multi_image_ids -2 -1)
    list(APPEND dfu_multi_image_paths "${s0_bin_path}" "${s1_bin_path}")
    list(APPEND dfu_multi_image_targets signed_s0_target signed_s1_target)
  endif()

  dfu_multi_image_package(dfu_multi_image_pkg
    IMAGE_IDS ${dfu_multi_image_ids}
    IMAGE_PATHS ${dfu_multi_image_paths}
    OUTPUT ${PROJECT_BINARY_DIR}/dfu_multi_image.bin
    )

  add_dependencies(dfu_multi_image_pkg ${dfu_multi_image_targets})
endif()

if(CONFIG_ZIGBEE AND CONFIG_ZIGBEE_FOTA)
  if (CONFIG_ZIGBEE_FOTA_GENERATE_LEGACY_IMAGE_TYPE)
    set(firmware_binary "${PROJECT_BINARY_DIR}/${app_core_binary_name}")
    set(legacy_cmd "--legacy")
  elseif(CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD)
    set(firmware_binary "${PROJECT_BINARY_DIR}/dfu_multi_image.bin")
    set(legacy_cmd )
  else()
    message(FATAL_ERROR "No Zigbee FOTA image format selected. Please enable either legacy or the multi-image format.")
  endif()

  add_custom_target(zigbee_ota_image ALL
    COMMAND
    ${PYTHON_EXECUTABLE}
      ${NRF_DIR}/scripts/bootloader/zb_add_ota_header.py
      --application ${firmware_binary}
      --application-version-string ${CONFIG_MCUBOOT_IMAGE_VERSION}
      --zigbee-manufacturer-id ${CONFIG_ZIGBEE_FOTA_MANUFACTURER_ID}
      --zigbee-image-type ${CONFIG_ZIGBEE_FOTA_IMAGE_TYPE}
      --zigbee-comment ${CONFIG_ZIGBEE_FOTA_COMMENT}
      --zigbee-ota-min-hw-version ${CONFIG_ZIGBEE_FOTA_MIN_HW_VERSION}
      --zigbee-ota-max-hw-version ${CONFIG_ZIGBEE_FOTA_MAX_HW_VERSION}
      --out-directory ${PROJECT_BINARY_DIR}
      ${legacy_cmd}

    DEPENDS
    ${firmware_binary}
  )
endif(CONFIG_ZIGBEE AND CONFIG_ZIGBEE_FOTA)

# Zephyr has a Kconfig option used for signing an application image
# with MCUboot using west sign. If partition manager is in use and
# there are multiple images, we want to make sure users understand
# this option should probably be left alone, since the NCS build
# system has its own way of managing signing.
if (CONFIG_MCUBOOT_SIGNATURE_KEY_FILE)
    message(WARNING
      "CONFIG_MCUBOOT_SIGNATURE_KEY_FILE is set to \"${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}\".
You are using the NCS Mcuboot signing, which means this option will be ignored.
Image signing in NCS is done via the MCUboot image's \
 CONFIG_BOOT_SIGNATURE_KEY_FILE option.
Consider setting CONFIG_MCUBOOT_SIGNATURE_KEY_FILE in your application image\
 back to its default value, the empty string.")
endif()

# NCS Handles everything regarding mcuboot, ensure Zephyr doesn't interfere.
# This is a temporary solution until Zephyr signing has been made more modular.
set(CONFIG_BOOTLOADER_MCUBOOT False PARENT_SCOPE)
