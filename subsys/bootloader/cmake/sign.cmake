#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(GENERATED_PATH ${PROJECT_BINARY_DIR}/nrf/subsys/bootloader/generated)

# This is needed for make, ninja is able to resolve and create the path but make
# is not able to resolve it.
file(MAKE_DIRECTORY ${GENERATED_PATH})

set(SIGNATURE_PUBLIC_KEY_FILE ${GENERATED_PATH}/public.pem)

if (CONFIG_SB_SIGNING_PYTHON)
  set(PUB_GEN_CMD
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/keygen.py
    --public
    --in ${SIGNATURE_PRIVATE_KEY_FILE}
    --out ${SIGNATURE_PUBLIC_KEY_FILE}
    )
elseif (CONFIG_SB_SIGNING_OPENSSL)
  set(PUB_GEN_CMD
    openssl ec
    -pubout
    -in ${SIGNATURE_PRIVATE_KEY_FILE}
    -out ${SIGNATURE_PUBLIC_KEY_FILE}
    )
elseif (CONFIG_SB_SIGNING_CUSTOM)
  set(SIGNATURE_PUBLIC_KEY_FILE ${CONFIG_SB_SIGNING_PUBLIC_KEY})
  if (NOT EXISTS ${SIGNATURE_PUBLIC_KEY_FILE} OR IS_DIRECTORY ${SIGNATURE_PUBLIC_KEY_FILE})
    message(WARNING "Invalid public key file: ${SIGNATURE_PUBLIC_KEY_FILE}")
  endif()
else ()
  message(WARNING "Unable to parse signing config.")
endif()

if (CONFIG_SB_PRIVATE_KEY_PROVIDED)
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
endif()

# Public key file target is required for all signing options
add_custom_target(
    signature_public_key_file_target
    DEPENDS
    ${SIGNATURE_PUBLIC_KEY_FILE}
  )

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/bl_validation_magic.cmake)

set(slots s0_image)
if (CONFIG_SECURE_BOOT AND CONFIG_SOC_NRF5340_CPUNET AND NOT NCS_SYSBUILD_PARTITION_MANAGER)
  list(APPEND slots app)
endif()

if(NCS_SYSBUILD_PARTITION_MANAGER AND "${SB_CONFIG_SECURE_BOOT_DOMAIN}" STREQUAL "CPUNET")
  set(slots ${DOMAIN_APP_${SB_CONFIG_SECURE_BOOT_DOMAIN}})
endif()

if (CONFIG_BUILD_S1_VARIANT)
  list(APPEND slots s1_image)
endif ()

if (NOT "${CONFIG_SB_VALIDATION_INFO_CRYPTO_ID}" EQUAL "1")
  message(FATAL_ERROR
    "This value of SB_VALIDATION_INFO_CRYPTO_ID is not supported")
endif()

foreach (slot ${slots})
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

    set(slot_hex ${${slot}_image_dir}/zephyr/${${slot}_kernel_name}.hex)
    set(sign_depends ${${slot}_image_dir}/zephyr/${${slot}_kernel_name}.hex)
  else()
    set(slot_hex ${PROJECT_BINARY_DIR}/${slot}.hex)
    set(sign_depends ${slot}_hex)
  endif()
else()
  if (${slot} STREQUAL "s1_image")
    # The s1_image slot is built as a child image, add the dependency and
    # path to its hex file accordingly. We cannot use the shared variables
    # from the s1 child image since its configure stage might not have executed
    # yet.
    set(slot_hex ${CMAKE_BINARY_DIR}/s1_image/zephyr/zephyr.hex)
    set(sign_depends s1_image_subimage)
  else()
    set(slot_hex ${PROJECT_BINARY_DIR}/${slot}.hex)
    set(sign_depends ${slot}_hex)
  endif()
  list(APPEND sign_depends ${slot_hex} ${SIGN_KEY_FILE_DEPENDS})
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

  if (CONFIG_SB_SIGNING_PYTHON)
    set(sign_cmd
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/do_sign.py
      --private-key ${SIGNATURE_PRIVATE_KEY_FILE}
      --in ${hash_file}
      > ${signature_file}
      )
  elseif (CONFIG_SB_SIGNING_OPENSSL)
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
  elseif (CONFIG_SB_SIGNING_CUSTOM)
    set(custom_sign_cmd "${CONFIG_SB_SIGNING_COMMAND}")

    if ((custom_sign_cmd STREQUAL "") OR (NOT EXISTS ${SIGNATURE_PUBLIC_KEY_FILE}))
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
    --offset ${CONFIG_SB_VALIDATION_METADATA_OFFSET}
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
    "Creating validation for ${KERNEL_HEX_NAME}, storing to ${SIGNED_KERNEL_HEX_NAME}"
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
    ${slot}_PM_HEX_FILE
    ${signed_hex}
    )

  set_property(
    GLOBAL PROPERTY
    ${slot}_PM_TARGET
    ${slot}_signed_kernel_hex_target
    )

endforeach()


if (CONFIG_BUILD_S1_VARIANT AND NOT CONFIG_BOOTLOADER_MCUBOOT)
  # B0 will boot the app directly, create the DFU zip. If MCUBoot is enabled,
  # the DFU zip files is generated by the MCUBoot build code.

  # Create dependency to ensure explicit build order. This is needed to have
  # a single target represent the state when both s0 and s1 imags are built.
  add_dependencies(
    s1_image_signed_kernel_hex_target
    s0_image_signed_kernel_hex_target
    )

  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

  # Generate zip file with both update candidates
  set(s0_name signed_by_b0_s0_image.bin)
  set(s0_bin_path ${PROJECT_BINARY_DIR}/${s0_name})
  set(s1_name signed_by_b0_s1_image.bin)
  set(s1_bin_path ${PROJECT_BINARY_DIR}/${s1_name})

  generate_dfu_zip(
    OUTPUT ${PROJECT_BINARY_DIR}/dfu_application.zip
    BIN_FILES ${s0_bin_path} ${s1_bin_path}
    TYPE application
    SCRIPT_PARAMS
    "${s0_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>"
    "${s1_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>"
    "version_B0=${CONFIG_FW_INFO_FIRMWARE_VERSION}"
    )
endif()
