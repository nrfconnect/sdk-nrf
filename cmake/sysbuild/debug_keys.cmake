#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

string(REPLACE "," ";" PUBLIC_KEY_FILES_LIST "${SB_CONFIG_SECURE_BOOT_PUBLIC_KEY_FILES}")

set(PRIV_CMD
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/keygen.py --private
  )

set(PUB_CMD
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/keygen.py --public
  )

# Check if PEM file is specified by user, if not, create one.

# First, check environment variables. Only use if not specified in command line.
if(DEFINED ENV{SB_SIGNING_KEY_FILE} AND NOT SB_SIGNING_KEY_FILE)
  if(NOT EXISTS "$ENV{SB_SIGNING_KEY_FILE}")
    message(FATAL_ERROR "ENV points to non-existing PEM file '$ENV{SB_SIGNING_KEY_FILE}'")
  else()
    set(SIGNATURE_PRIVATE_KEY_FILE $ENV{SB_SIGNING_KEY_FILE})
  endif()
endif()

# Next, check command line arguments
if(DEFINED SB_SIGNING_KEY_FILE)
  set(SIGNATURE_PRIVATE_KEY_FILE ${SB_SIGNING_KEY_FILE})
endif()

# Check if debug sign key should be generated.
if("${SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE}" STREQUAL "")
  message(WARNING "
    --------------------------------------------------------------
    --- WARNING: Using generated NSIB public/private key-pair. ---
    --- It should not be used for production.                  ---
    --- See SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE             ---
    --------------------------------------------------------------
    \n"
  )

  set(DEBUG_SIGN_KEY ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem)
  set(SIGNATURE_PRIVATE_KEY_FILE ${DEBUG_SIGN_KEY})
  add_custom_command(
    OUTPUT
    ${DEBUG_SIGN_KEY}
    COMMAND
    ${PRIV_CMD}
    --out ${DEBUG_SIGN_KEY}
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMENT
    "Generating signing key"
    USES_TERMINAL
    )
  add_custom_target(
    debug_sign_key_target
    DEPENDS
    ${DEBUG_SIGN_KEY}
    )
  set(SIGN_KEY_FILE_DEPENDS debug_sign_key_target)
else()
  # Resolve path.
  if(IS_ABSOLUTE ${SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE})
    set(SIGNATURE_PRIVATE_KEY_FILE ${SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE})
  else()
    set(SIGNATURE_PRIVATE_KEY_FILE
      ${APPLICATION_CONFIG_DIR}/${SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE})
  endif()

  if(NOT EXISTS ${SIGNATURE_PRIVATE_KEY_FILE})
    message(FATAL_ERROR "Config points to non-existing PEM file '${SIGNATURE_PRIVATE_KEY_FILE}'")
  endif()
endif()

set(PUBLIC_KEY_FILES "")

# Check if debug public and private keys for key revokation should be generated
if("${SB_CONFIG_SECURE_BOOT_PUBLIC_KEY_FILES}" STREQUAL "debug")
  message(WARNING "
    ---------------------------------------------------------------
    --- WARNING: Using generated NSIB public/private key-pairs. ---
    --- They should not be used for production.                 ---
    --- See SB_CONFIG_SECURE_BOOT_PUBLIC_KEY_FILES              ---
    ---------------------------------------------------------------
    \n"
    )

  set(debug_public_key_0 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PUBLIC_0.pem)
  set(debug_private_key_0 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PRIVATE_0.pem)
  set(debug_public_key_1 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PUBLIC_1.pem)
  set(debug_private_key_1 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PRIVATE_1.pem)

  list(APPEND PUBLIC_KEY_FILES ${debug_public_key_0} ${debug_public_key_1})

  add_custom_command(
    OUTPUT
    ${debug_public_key_0}
    ${debug_private_key_0}
    ${debug_public_key_1}
    ${debug_private_key_1}
    COMMAND
    ${PRIV_CMD}
    --out ${debug_private_key_0}
    COMMAND
    ${PRIV_CMD}
    --out ${debug_private_key_1}
    COMMAND
    ${PUB_CMD}
    --in ${debug_private_key_0}
    --out ${debug_public_key_0}
    COMMAND
    ${PUB_CMD}
    --in ${debug_private_key_1}
    --out ${debug_public_key_1}
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMENT
    "Generating extra provision key files"
    USES_TERMINAL
    )
  add_custom_target(
    provision_key_target
    DEPENDS
    ${debug_public_key_0}
    ${debug_private_key_0}
    ${debug_public_key_1}
    ${debug_private_key_0}
    )
  set(PROVISION_KEY_DEPENDS provision_key_target)
else()
  foreach(key ${PUBLIC_KEY_FILES_LIST})
    # Resolve path.
    if(IS_ABSOLUTE ${key})
      list(APPEND PUBLIC_KEY_FILES ${key})
    else()
      list(APPEND PUBLIC_KEY_FILES ${CMAKE_SOURCE_DIR}/${key})
    endif()
  endforeach()
endif()
