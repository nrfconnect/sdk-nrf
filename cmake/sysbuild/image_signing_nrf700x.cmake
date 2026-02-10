# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(nrf7x_signing_tasks input output_hex output_bin dependencies)
  set(keyfile "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  set(keyfile_enc "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")
  string(CONFIGURE "${keyfile}" keyfile)
  string(CONFIGURE "${keyfile_enc}" keyfile_enc)

  if(NOT "${SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE}")
    # Check for misconfiguration.
    if("${keyfile}" STREQUAL "")
      # No signature key file, no signed binaries. No error, though:
      # this is the documented behavior.
      message(WARNING "Neither SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE or "
                      "SB_CONFIG_BOOT_SIGNATURE_KEY_FILE are set, the generated build will not be "
                      "bootable by MCUboot unless it is signed manually/externally.")
      return()
    endif()
  endif()

  # No imgtool, no signed binaries.
  if(NOT DEFINED IMGTOOL)
    message(FATAL_ERROR "Can't sign images for MCUboot: can't find imgtool. To fix, install "
      "imgtool with pip3, or add the mcuboot repository to the west manifest and ensure it "
      "has a scripts/imgtool.py file."
    )
    return()
  endif()

  # Fetch devicetree details for flash and slot information
  dt_chosen(flash_node TARGET ${DEFAULT_IMAGE} PROPERTY "zephyr,flash")
  dt_nodelabel(slot0_flash TARGET ${DEFAULT_IMAGE} NODELABEL "slot0_partition" REQUIRED)
  dt_prop(slot_size TARGET ${DEFAULT_IMAGE} PATH "${slot0_flash}" PROPERTY "reg" INDEX 1 REQUIRED)
  dt_prop(write_block_size TARGET ${DEFAULT_IMAGE} PATH "${flash_node}" PROPERTY "write-block-size")

  if(NOT write_block_size)
    set(write_block_size 4)
    message(WARNING "slot0_partition write block size devicetree parameter is missing, assuming write block size is 4")
  endif()

  if(SB_CONFIG_PARTITION_MANAGER)
    sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION IMAGE ${DEFAULT_IMAGE} VAR CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG)
    set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}
      --align ${write_block_size} --slot-size $<TARGET_PROPERTY:partition_manager,PM_NRF70_WIFI_FW_SIZE>
      --pad-header --header-size ${SB_CONFIG_PM_MCUBOOT_PAD}
    )
    sysbuild_get(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION IMAGE ${DEFAULT_IMAGE} VAR
      CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION KCONFIG
    )
  else()
    dt_nodelabel(nrf70_wifi_fw_partition_nodelabel TARGET ${DEFAULT_IMAGE} NODELABEL
      "nrf70_wifi_fw_partition"
    )
    dt_reg_size(nrf70_wifi_fw_partition_size TARGET ${DEFAULT_IMAGE} PATH
      "${nrf70_wifi_fw_partition_nodelabel}"
    )
    sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION IMAGE ${DEFAULT_IMAGE} VAR
      CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG
    )
    sysbuild_get(CONFIG_ROM_START_OFFSET IMAGE ${DEFAULT_IMAGE} VAR CONFIG_ROM_START_OFFSET
      KCONFIG
    )
    set(imgtool_sign ${PYTHON_EXECUTABLE} ${IMGTOOL} sign --version
      ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --align ${write_block_size} --slot-size
      ${nrf70_wifi_fw_partition_size} --pad-header --header-size ${CONFIG_ROM_START_OFFSET}
    )
    sysbuild_get(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION IMAGE ${DEFAULT_IMAGE} VAR
      CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION KCONFIG
    )
  endif()

  if(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    set(imgtool_extra --security-counter ${CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE})
  endif()

  if(NOT "${keyfile}" STREQUAL "")
    set(imgtool_extra -k "${keyfile}" ${imgtool_extra})
  endif()

  set(imgtool_args ${imgtool_extra})

  # 'west sign' arguments for confirmed, unconfirmed and encrypted images.
  set(unconfirmed_args)

  # Set up outputs
  if(SB_CONFIG_PARTITION_MANAGER)
    add_custom_target(nrf70_wifi_fw_patch_signed_target
      DEPENDS ${output_hex} ${output_bin}
    )
  else()
    add_custom_target(nrf70_wifi_fw_patch_signed_target
      ALL
      DEPENDS ${output_hex} ${output_bin}
    )
  endif()

  add_custom_command(OUTPUT ${output_hex}
    COMMAND
    ${imgtool_sign} ${imgtool_args} ${input} ${output_hex}
    DEPENDS ${input} ${dependencies}
  )

  add_custom_command(OUTPUT ${output_bin}
    COMMAND
    ${imgtool_sign} ${imgtool_args} ${input} ${output_bin}
    DEPENDS ${input} ${dependencies}
  )

#  if(SB_CONFIG_BOOT_ENCRYPTION)
#    set(unconfirmed_args ${input}.hex ${output}.encrypted.hex)
#    list(APPEND byproducts ${output}.encrypted.hex)
#
#    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
#      ${imgtool_sign} ${imgtool_args} --encrypt "${keyfile_enc}" ${unconfirmed_args})
#  endif()
endfunction()
