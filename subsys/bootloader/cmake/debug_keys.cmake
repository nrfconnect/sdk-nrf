#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#


set(privcmd
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/keygen.py --private
  )

set(pubcmd
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/keygen.py --public
  )

# Check if PEM file is specified by user, if not, create one.

# First, check environment variables. Only use if not specified in command line.
if (DEFINED ENV{SB_SIGNING_KEY_FILE} AND NOT SB_SIGNING_KEY_FILE)
  if (NOT EXISTS "$ENV{SB_SIGNING_KEY_FILE}")
    message(FATAL_ERROR "ENV points to non-existing PEM file '$ENV{SB_SIGNING_KEY_FILE}'")
  else()
    set(SIGNATURE_PRIVATE_KEY_FILE $ENV{SB_SIGNING_KEY_FILE})
  endif()
endif()

# Next, check command line arguments
if (DEFINED SB_SIGNING_KEY_FILE)
  set(SIGNATURE_PRIVATE_KEY_FILE ${SB_SIGNING_KEY_FILE})
endif()

# Lastly, check if debug keys should be used.
if( "${CONFIG_SB_SIGNING_KEY_FILE}" STREQUAL "")
  set(debug_sign_key ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem)
  set(SIGNATURE_PRIVATE_KEY_FILE ${debug_sign_key})
  set(KEY_FILE_DEPENDS ${SIGNATURE_PRIVATE_KEY_FILE})
  add_custom_command(
    OUTPUT
    ${debug_sign_key}
    COMMAND
    ${privcmd}
    --out ${debug_sign_key}
    DEPENDS kernel_elf
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMENT
    "Generating signing key"
    USES_TERMINAL
    )
else()
  if (NOT EXISTS "${CONFIG_SB_SIGNING_KEY_FILE}")
    message(FATAL_ERROR "Config points to non-existing PEM file '${CONFIG_SB_SIGNING_KEY_FILE}'")
  endif()
  set(SIGNATURE_PRIVATE_KEY_FILE ${CONFIG_SB_SIGNING_KEY_FILE})
endif()

if ("${CONFIG_SB_PUBLIC_KEY_FILES}" STREQUAL "")
  set(debug_public_key_0 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PUBLIC_0.pem)
  set(debug_private_key_0 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PRIVATE_0.pem)
  set(debug_public_key_1 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PUBLIC_1.pem)
  set(debug_private_key_1 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PRIVATE_1.pem)
  set (PUBLIC_KEY_FILES "${debug_public_key_0},${debug_public_key_1}")
  set (PROVISION_DEPENDS  ${debug_public_key_0} ${debug_public_key_1})
  add_custom_command(
    OUTPUT
    ${debug_public_key_0}
    ${debug_private_key_0}
    ${debug_public_key_1}
    ${debug_private_key_1}
    COMMAND
    ${privcmd}
    --out ${debug_private_key_0}
    COMMAND
    ${privcmd}
    --out ${debug_private_key_1}
    COMMAND
    ${pubcmd}
    --in ${debug_private_key_0}
    --out ${debug_public_key_0}
    COMMAND
    ${pubcmd}
    --in ${debug_private_key_1}
    --out ${debug_public_key_1}
    DEPENDS kernel_elf
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMENT
    "Generating extra provision key files"
    USES_TERMINAL
    )
else ()
  # TODO see if we can use some generator expression to avoid using 'kerne_elf' directly.
  set (PUBLIC_KEY_FILES ${CONFIG_SB_PUBLIC_KEY_FILES})
  set (PROVISION_DEPENDS kernel_elf)
endif()
