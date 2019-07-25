#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#


#TODO: Detect which elf to depend on and resolve which
#      image should be hashed and signed by b0.

set(KERNEL_ELF_TO_BE_SIGNED zephyr_final)

set(GENERATED_FILES_PATH ${PROJECT_BINARY_DIR}/nrf/subsys/bootloader/generated)

# This is needed for make, ninja is able to resolve and create the path but make
# are not able to resolve it.
file(MAKE_DIRECTORY ${GENERATED_FILES_PATH})

set(HASH_FILE ${GENERATED_FILES_PATH}/firmware.sha256)
set(SIGNATURE_FILE ${GENERATED_FILES_PATH}/firmware.signature)
set(SIGNATURE_PUBLIC_KEY_FILE ${GENERATED_FILES_PATH}/public.pem)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/fw_info_magic.cmake)

set(HASH_CMD
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/hash.py
  --in ${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}
  > ${HASH_FILE}
  )

if (CONFIG_SB_SIGNING_PYTHON)
  set(SIGN_CMD
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/do_sign.py
    --private-key ${SIGNATURE_PRIVATE_KEY_FILE}
    --in ${HASH_FILE}
    > ${SIGNATURE_FILE}
    )
  set(PUB_GEN_CMD
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/keygen.py
    --public
    --in ${SIGNATURE_PRIVATE_KEY_FILE}
    --out ${SIGNATURE_PUBLIC_KEY_FILE}
    )
elseif (CONFIG_SB_SIGNING_OPENSSL)
  set(SIGN_CMD
    openssl dgst
    -sha256
    -sign ${SIGNATURE_PRIVATE_KEY_FILE} ${HASH_FILE} |
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/asn1parse.py
    --alg ecdsa
    --contents signature
    > ${SIGNATURE_FILE}
    )
  set(PUB_GEN_CMD
    openssl ec
    -pubout
    -in ${SIGNATURE_PRIVATE_KEY_FILE}
    -out ${SIGNATURE_PUBLIC_KEY_FILE}
    )
elseif (CONFIG_SB_SIGNING_CUSTOM)
  set(SIGN_CMD
    ${CONFIG_SB_SIGNING_COMMAND}
    ${HASH_FILE}
    > ${SIGNATURE_FILE}
    )
  set(SIGNATURE_PUBLIC_KEY_FILE ${CONFIG_SB_SIGNING_PUBLIC_KEY})
  if (SIGN_CMD STREQUAL "" OR NOT EXISTS ${SIGNATURE_PUBLIC_KEY_FILE})
    message(WARNING "You must specify a signing command and valid public key file.")
  endif()
else ()
  message(WARNING "Unable to parse signing config.")
endif()

add_custom_command(
  OUTPUT
  ${SIGNATURE_FILE}
  COMMAND
  ${HASH_CMD}
  COMMAND
  ${SIGN_CMD}
  DEPENDS
  ${KERNEL_ELF_TO_BE_SIGNED}
  WORKING_DIRECTORY
  ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating signature of application"
  USES_TERMINAL
  )

add_custom_target(
  signature_file_target
  DEPENDS
  ${SIGNATURE_FILE}
  )

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

add_custom_command(
  OUTPUT
  ${SIGNED_KERNEL_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/validation_data.py
  --input ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
  --output ${SIGNED_KERNEL_HEX}
  --offset ${CONFIG_FW_VALIDATION_METADATA_OFFSET}
  --signature ${SIGNATURE_FILE}
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
  ${SIGNED_KERNEL_HEX}
  signature_file_target
  signature_public_key_file_target
  )
