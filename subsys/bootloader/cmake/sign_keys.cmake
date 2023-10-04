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
