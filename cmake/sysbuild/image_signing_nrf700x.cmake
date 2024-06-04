# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(nrf7x_signing_tasks input output_hex output_bin dependencies)
  set(keyfile "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  set(keyfile_enc "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")

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

  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION IMAGE ${DEFAULT_IMAGE} VAR CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG)
  set(imgtool_sign ${PYTHON_EXECUTABLE} ${imgtool_path} sign --version ${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION} --align 4 --slot-size $<TARGET_PROPERTY:partition_manager,PM_NRF70_WIFI_FW_SIZE> --pad-header --header-size ${SB_CONFIG_PM_MCUBOOT_PAD})

  sysbuild_get(CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION IMAGE ${DEFAULT_IMAGE} VAR CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION KCONFIG)
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
    add_custom_target(nrf70_wifi_fw_patch_signed_target
      DEPENDS ${output_hex} ${output_bin}
    )

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
