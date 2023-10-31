#
# Copyright (c) 2018-2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(b0_gen_keys)
  set(GENERATED_PATH ${PROJECT_BINARY_DIR}/nrf/subsys/bootloader/generated)

  # This is needed for make, ninja is able to resolve and create the path but make
  # is not able to resolve it.
  file(MAKE_DIRECTORY ${GENERATED_PATH})

  set(SIGNATURE_PUBLIC_KEY_FILE ${GENERATED_PATH}/public.pem)
  set(SIGNATURE_PUBLIC_KEY_FILE ${GENERATED_PATH}/public.pem PARENT_SCOPE)

  if(SB_CONFIG_SECURE_BOOT_SIGNING_PYTHON)
    set(PUB_GEN_CMD
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/keygen.py
      --public
      --in ${SIGNATURE_PRIVATE_KEY_FILE}
      --out ${SIGNATURE_PUBLIC_KEY_FILE}
      )
  elseif(SB_CONFIG_SECURE_BOOT_SIGNING_OPENSSL)
    set(PUB_GEN_CMD
      openssl ec
      -pubout
      -in ${SIGNATURE_PRIVATE_KEY_FILE}
      -out ${SIGNATURE_PUBLIC_KEY_FILE}
      )
  elseif(SB_CONFIG_SECURE_BOOT_SIGNING_CUSTOM)
    set(SIGNATURE_PUBLIC_KEY_FILE ${SB_CONFIG_SECURE_BOOT_SIGNING_PUBLIC_KEY})
    set(SIGNATURE_PUBLIC_KEY_FILE ${SB_CONFIG_SECURE_BOOT_SIGNING_PUBLIC_KEY} PARENT_SCOPE)
    if(NOT EXISTS ${SIGNATURE_PUBLIC_KEY_FILE} OR IS_DIRECTORY ${SIGNATURE_PUBLIC_KEY_FILE})
      message(WARNING "Invalid public key file: ${SIGNATURE_PUBLIC_KEY_FILE}")
    endif()
  else()
    message(WARNING "Unable to parse signing config.")
    return()
  endif()

# TODO: this config is gone
#if(SB_CONFIG_SECURE_BOOT_PRIVATE_KEY_PROVIDED)
  add_custom_command(
    OUTPUT
    ${SIGNATURE_PUBLIC_KEY_FILE}
    COMMAND
    ${PUB_GEN_CMD}
    DEPENDS
    ${SIGNATURE_PRIVATE_KEY_FILE}
    COMMENT
    "Creating public key from private key used for signing"
    WORKING_DIRECTORY
    ${PROJECT_BINARY_DIR}
    USES_TERMINAL
    )
#endif()

  # Public key file target is required for all signing options
  add_custom_target(
      signature_public_key_file_target
      DEPENDS
      ${SIGNATURE_PUBLIC_KEY_FILE}
    )
endfunction()

function(b0_sign_image slot)
  set(GENERATED_PATH ${PROJECT_BINARY_DIR}/nrf/subsys/bootloader/generated)
  set(SIGNATURE_PUBLIC_KEY_FILE ${GENERATED_PATH}/public.pem)

  # Get variables for secure boot usage
  sysbuild_get(${slot}_sb_validation_info_version IMAGE ${slot} VAR CONFIG_SB_VALIDATION_INFO_VERSION KCONFIG)
  sysbuild_get(${slot}_fw_info_hardware_id IMAGE ${slot} VAR CONFIG_FW_INFO_HARDWARE_ID KCONFIG)
  sysbuild_get(${slot}_sb_validation_info_crypto_id IMAGE ${slot} VAR CONFIG_SB_VALIDATION_INFO_CRYPTO_ID KCONFIG)
  sysbuild_get(${slot}_fw_info_magic_compatibility_id IMAGE ${slot} VAR CONFIG_FW_INFO_MAGIC_COMPATIBILITY_ID KCONFIG)
  sysbuild_get(${slot}_fw_info_magic_common IMAGE ${slot} VAR CONFIG_FW_INFO_MAGIC_COMMON KCONFIG)
  sysbuild_get(${slot}_sb_validation_info_magic IMAGE ${slot} VAR CONFIG_SB_VALIDATION_INFO_MAGIC KCONFIG)

  math(EXPR
    MAGIC_COMPATIBILITY_VALIDATION_INFO
    "(${${slot}_sb_validation_info_version}) |
     (${${slot}_fw_info_hardware_id} << 8) |
     (${${slot}_sb_validation_info_crypto_id} << 16) |
     (${${slot}_fw_info_magic_compatibility_id} << 24)"
    )

  set(VALIDATION_INFO_MAGIC    "${${slot}_fw_info_magic_common},${${slot}_sb_validation_info_magic},${MAGIC_COMPATIBILITY_VALIDATION_INFO}")

  set(signed_hex ${PROJECT_BINARY_DIR}/signed_by_b0_${slot}.hex)
  set(signed_bin ${PROJECT_BINARY_DIR}/signed_by_b0_${slot}.bin)

  if(NCS_SYSBUILD_PARTITION_MANAGER)
    # A container can be merged, in which case we should use old style below,
    # or it may be an actual image, where we know everything.
    # Initial support disregards the merged hex files.
    # In parent-child, everything is merged, even when having a single image in a
    # container (where the original image == the merged image).
    if(TARGET ${slot})
      # If slot is a target of it's own, then it means we target the hex directly and not a merged hex.
      sysbuild_get(${slot}_image_dir IMAGE ${slot} VAR APPLICATION_BINARY_DIR CACHE)
      sysbuild_get(${slot}_kernel_name IMAGE ${slot} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
      sysbuild_get(${slot}_kernel_elf IMAGE ${slot} VAR CONFIG_KERNEL_ELF_NAME KCONFIG)
      sysbuild_get(${slot}_crypto_id IMAGE ${slot} VAR CONFIG_SB_VALIDATION_INFO_CRYPTO_ID KCONFIG)
      sysbuild_get(${slot}_validation_offset IMAGE ${slot} VAR CONFIG_SB_VALIDATION_METADATA_OFFSET KCONFIG)

      set(slot_hex ${${slot}_image_dir}/zephyr/${${slot}_kernel_name}.hex)
      set(sign_depends ${${slot}_image_dir}/zephyr/${${slot}_kernel_name}.elf)
      set(target_name ${slot})
    elseif("${slot}" STREQUAL "s0_image")
      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        set(target_name mcuboot)
      else()
        set(target_name ${DEFAULT_IMAGE})
      endif()

      sysbuild_get(${target_name}_image_dir IMAGE ${target_name} VAR APPLICATION_BINARY_DIR CACHE)
      sysbuild_get(${target_name}_kernel_name IMAGE ${target_name} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
      sysbuild_get(${slot}_crypto_id IMAGE ${target_name} VAR CONFIG_SB_VALIDATION_INFO_CRYPTO_ID KCONFIG)
      sysbuild_get(${slot}_validation_offset IMAGE ${target_name} VAR CONFIG_SB_VALIDATION_METADATA_OFFSET KCONFIG)

      set(slot_hex ${${target_name}_image_dir}/zephyr/${${target_name}_kernel_name}.hex)
      set(sign_depends ${target_name} ${${target_name}_image_dir}/zephyr/${${target_name}_kernel_name}.elf)
    else()
      message(FATAL_ERROR "Not supported")
    endif()
  else()
    message(FATAL_ERROR "Not supported")
  endif()

  if(NOT "${${slot}_crypto_id}" EQUAL "1")
    message(FATAL_ERROR
      "This value of SB_VALIDATION_INFO_CRYPTO_ID is not supported")
  endif()

  set(to_sign ${slot_hex})
  set(hash_file ${GENERATED_PATH}/${slot}_firmware.sha256)
  set(signature_file ${GENERATED_PATH}/${slot}_firmware.signature)

  set(hash_cmd
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/hash.py
    --in ${to_sign}
    > ${hash_file}
    )

  if(SB_CONFIG_SECURE_BOOT_SIGNING_PYTHON)
    set(sign_cmd
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/do_sign.py
      --private-key ${SIGNATURE_PRIVATE_KEY_FILE}
      --in ${hash_file}
      > ${signature_file}
      )
  elseif(SB_CONFIG_SECURE_BOOT_SIGNING_OPENSSL)
    set(sign_cmd
      openssl dgst
      -sha256
      -sign ${SIGNATURE_PRIVATE_KEY_FILE} ${hash_file} |
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/asn1parse.py
      --alg ecdsa
      --contents signature
      > ${signature_file}
      )
  elseif(SB_CONFIG_SECURE_BOOT_SIGNING_CUSTOM)
    set(custom_sign_cmd "${SB_CONFIG_SECURE_BOOT_SIGNING_COMMAND}")

    if (("${custom_sign_cmd}" STREQUAL "") OR (NOT EXISTS ${SIGNATURE_PUBLIC_KEY_FILE}))
      message(FATAL_ERROR "You must specify a signing command and valid public key file for custom signing.")
    endif()

    string(APPEND custom_sign_cmd " ${hash_file} > ${signature_file}")
    string(REPLACE " " ";" sign_cmd ${custom_sign_cmd})
  else ()
    message(WARNING "Unable to parse signing config.")
  endif()

  add_custom_command(
    OUTPUT
    ${signature_file}
    COMMAND
    ${hash_cmd}
    COMMAND
    ${sign_cmd}
    DEPENDS
    ${sign_depends}
    WORKING_DIRECTORY
    ${PROJECT_BINARY_DIR}
    COMMENT
    "Creating signature of application"
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
    )

  add_custom_target(
    ${slot}_signature_file_target
    DEPENDS
    ${signature_file}
    )

  cmake_path(GET signed_hex FILENAME signed_hex_filename)
  cmake_path(GET to_sign FILENAME to_sign_filename)
  set(validation_comment "Creating validation for ${to_sign_filename}, storing to ${signed_hex_filename}")

  add_custom_command(
    OUTPUT
    ${signed_hex}
    ${signed_bin}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/validation_data.py
    --input ${to_sign}
    --output-hex ${signed_hex}
    --output-bin ${signed_bin}
    --offset ${${slot}_validation_offset}
    --signature ${signature_file}
    --public-key ${SIGNATURE_PUBLIC_KEY_FILE}
    --magic-value "${VALIDATION_INFO_MAGIC}"
    DEPENDS
    ${SIGN_KEY_FILE_DEPENDS}
    ${signature_file}
    ${slot}_signature_file_target
    ${SIGNATURE_PUBLIC_KEY_FILE}
    WORKING_DIRECTORY
    ${PROJECT_BINARY_DIR}
    COMMENT
    "${validation_comment}"
    USES_TERMINAL
    )

  add_custom_target(
    ${slot}_signed_kernel_hex_target
    DEPENDS
    ${signed_hex}
    ${slot}_signature_file_target
    signature_public_key_file_target
    )

  # Set hex file and target for the ${slot) (s0/s1) container partition.
  # This includes the hex file (and its corresponding target) to the build.
  set_property(
    GLOBAL PROPERTY
    ${target_name}_PM_HEX_FILE
    ${signed_hex}
    )

  set_property(
    GLOBAL PROPERTY
    ${target_name}_PM_TARGET
    ${slot}_signed_kernel_hex_target
    )
endfunction()
