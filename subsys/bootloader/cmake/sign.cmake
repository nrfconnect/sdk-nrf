#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(sign_depends kernel_elf ${KEY_FILE_DEPENDS})

set(hash_file firmware.sha256)
set(signature_file firmware.signature)
set(SIGNATURE_PUBLIC_KEY_FILE public.pem)

set(hashcmd
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/hash.py
  --in ${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}
  > ${hash_file}
  )

if (CONFIG_SB_SIGNING_PYTHON)
  set(signcmd
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/do_sign.py
    --private-key ${SIGNATURE_PRIVATE_KEY_FILE}
    --in ${hash_file}
    > ${signature_file}
    )
  set(pubgencmd
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/keygen.py
    --public
    --in ${SIGNATURE_PRIVATE_KEY_FILE}
    --out ${SIGNATURE_PUBLIC_KEY_FILE}
    )
elseif (CONFIG_SB_SIGNING_OPENSSL)
  set(signcmd
    openssl dgst
    -sha256
    -sign ${SIGNATURE_PRIVATE_KEY_FILE} ${hash_file} |
    ${PYTHON_EXECUTABLE}
    ${NRF_BOOTLOADER_SCRIPTS}/asn1parse.py
    --alg ecdsa
    --contents signature
    > ${signature_file}
    )
  set(pubgencmd
    openssl ec
    -pubout
    -in ${SIGNATURE_PRIVATE_KEY_FILE}
    -out ${SIGNATURE_PUBLIC_KEY_FILE}
    )
elseif (CONFIG_SB_SIGNING_CUSTOM)
  set(signcmd
    ${CONFIG_SB_SIGNING_COMMAND}
    ${hash_file}
    > ${signature_file}
    )
  set(SIGNATURE_PUBLIC_KEY_FILE ${CONFIG_SB_SIGNING_PUBLIC_KEY})
  if (signcmd STREQUAL "" OR NOT EXISTS ${SIGNATURE_PUBLIC_KEY_FILE})
    message(WARNING "You must specify a signing command and valid public key file.")
  endif()
else ()
  message(WARNING "Unable to parse signing config.")
endif()

add_custom_command(
  OUTPUT
  ${signature_file}
  COMMAND
  ${hashcmd}
  COMMAND
  ${signcmd}
  DEPENDS
  ${sign_depends}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating signature of application"
  USES_TERMINAL
  )

if (CONFIG_SB_PRIVATE_KEY_PROVIDED)
  add_custom_command(
    OUTPUT
    ${SIGNATURE_PUBLIC_KEY_FILE}
    COMMAND
    ${pubgencmd}
    DEPENDS
    ${sign_depends}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT
    "Creating public key from private key used for signing"
    USES_TERMINAL
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
  --offset ${CONFIG_SB_VALIDATION_METADATA_OFFSET}
  --signature ${signature_file}
  --public-key ${SIGNATURE_PUBLIC_KEY_FILE}
  --magic-value "${VALIDATION_INFO_MAGIC}"
  --pk-hash-len ${CONFIG_SB_PUBLIC_KEY_HASH_LEN}
  DEPENDS
  ${sign_depends}
  ${signature_file}
  ${SIGNATURE_PUBLIC_KEY_FILE}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating validation for ${KERNEL_HEX_NAME}, storing to ${SIGNED_KERNEL_HEX_NAME}"
  USES_TERMINAL
  )

add_custom_target(
  signed_kernel_hex_target
  DEPENDS
  ${SIGNED_KERNEL_HEX}
  )
