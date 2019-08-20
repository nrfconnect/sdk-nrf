#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(GENERATED_PATH ${PROJECT_BINARY_DIR}/nrf/subsys/bootloader/generated)

# This is needed for make, ninja is able to resolve and create the path but make
# is not able to resolve it.
file(MAKE_DIRECTORY ${GENERATED_PATH})

set(SIGNATURE_PUBLIC_KEY_FILE ${GENERATED_PATH}/public.pem)

if (CONFIG_SB_SIGNING_PYTHON)
  set(PUB_GEN_CMD
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/keygen.py
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

  add_custom_target(
    signature_public_key_file_target
    DEPENDS
    ${SIGNATURE_PUBLIC_KEY_FILE}
  )
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/fw_info_magic.cmake)

  set(signed_hex ${PROJECT_BINARY_DIR}/signed_by_b0_s0.hex)
  set(sign_depends s0_hex)
  set(to_sign ${PROJECT_BINARY_DIR}/s0.hex)
  set(hash_file ${GENERATED_PATH}/s0_firmware.sha256)
  set(signature_file ${GENERATED_PATH}/s0_firmware.signature)

  set(hash_cmd
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/hash.py
    --in ${to_sign}
    > ${hash_file}
    )

if (CONFIG_SB_SIGNING_PYTHON)
  set(sign_cmd
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/do_sign.py
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
    ${NRF_BOOTLOADER_SCRIPTS}/asn1parse.py
    --alg ecdsa
    --contents signature
    > ${signature_file}
    )
elseif (CONFIG_SB_SIGNING_CUSTOM)
  set(sign_cmd
    ${CONFIG_SB_SIGNING_COMMAND}
    ${hash_file}
    > ${signature_file}
    )
  if (sign_cmd STREQUAL "" OR NOT EXISTS ${SIGNATURE_PUBLIC_KEY_FILE})
    message(WARNING "You must specify a signing command and valid public key file.")
  endif()
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
  )

add_custom_target(
  signature_file_target
  DEPENDS
  ${signature_file}
  )

add_custom_command(
  OUTPUT
  ${signed_hex}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/validation_data.py
  --input ${to_sign}
  --output ${signed_hex}
  --offset ${CONFIG_FW_VALIDATION_METADATA_OFFSET}
  --signature ${signature_file}
  --public-key ${SIGNATURE_PUBLIC_KEY_FILE}
  --magic-value "${VALIDATION_INFO_MAGIC}"
  DEPENDS
  ${SIGN_KEY_FILE_DEPENDS}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating validation for ${KERNEL_HEX_NAME}, storing to ${SIGNED_KERNEL_HEX_NAME}"
  USES_TERMINAL
  )

add_custom_target(
  signed_kernel_hex_target
  DEPENDS
  ${signed_hex}
  signature_file_target
  signature_public_key_file_target
  )

# Set hex file and target for the 's0' container partition. This includes the
# hex file (and its corresponding target) to the build.
set_property(
  GLOBAL PROPERTY
  s0_PM_HEX_FILE
  ${signed_hex}
)

set_property(
  GLOBAL PROPERTY
  s0_PM_TARGET
  signed_kernel_hex_target
)
