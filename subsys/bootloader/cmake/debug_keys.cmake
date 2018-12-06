#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(DEBUG_SIGN_KEY ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_0.pem)
set(DEBUG_PUBLIC_KEY_0 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PUBLIC_0.pem)
set(DEBUG_PUBLIC_KEY_1 ${PROJECT_BINARY_DIR}/GENERATED_NON_SECURE_PUBLIC_1.pem)

set(cmd
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/keygen.py
  )

add_custom_command(
  OUTPUT
  ${DEBUG_SIGN_KEY}
  ${DEBUG_PUBLIC_KEY_0}
  ${DEBUG_PUBLIC_KEY_1}
  COMMAND
  ${cmd}
  --output-private ${DEBUG_SIGN_KEY}
  COMMAND
  ${cmd}
  --output-public ${DEBUG_PUBLIC_KEY_0}
  COMMAND
  ${cmd}
  --output-public ${DEBUG_PUBLIC_KEY_1}
  DEPENDS kernel_elf
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT
  "Generating debug key files."
  USES_TERMINAL
  )

# Check if PEM file is specified by user, if not, create one.

# First, check environment variables. Only use if not specified in command line.
if (DEFINED ENV{SB_SIGNING_KEY_FILE} AND NOT SB_SIGNING_KEY_FILE)
  if (NOT EXISTS "$ENV{SB_SIGNING_KEY_FILE}")
    message(FATAL_ERROR "ENV points to non-existing PEM file '$ENV{SB_SIGNING_KEY_FILE}'")
  else()
    set(signature_private_key_file $ENV{SB_SIGNING_KEY_FILE})
  endif()
endif()

# Next, check command line arguments
if (DEFINED SB_SIGNING_KEY_FILE)
  set(signature_private_key_file ${SB_SIGNING_KEY_FILE})
endif()

# Lastly, check if debug keys should be used.
if( "${CONFIG_SB_SIGNING_KEY_FILE}" STREQUAL "")
  set(signature_private_key_file ${DEBUG_SIGN_KEY})
  set(key_file_depends ${signature_private_key_file})
else()
  if (NOT EXISTS "${CONFIG_SB_SIGNING_KEY_FILE}")
    message(FATAL_ERROR "Config points to non-existing PEM file '${CONFIG_SB_SIGNING_KEY_FILE}'")
  endif()
  set(signature_private_key_file ${CONFIG_SB_SIGNING_KEY_FILE})
endif()

if ("${CONFIG_SB_PUBLIC_KEY_FILES}" STREQUAL "")
  set (public_key_files "${DEBUG_PUBLIC_KEY_0},${DEBUG_PUBLIC_KEY_1}")
  set (provision_depends  ${DEBUG_PUBLIC_KEY_0} ${DEBUG_PUBLIC_KEY_1})
else ()
  # TODO see if we can use some generator expression to avoid using 'kerne_elf' directly.
  set (public_key_files ${CONFIG_SB_PUBLIC_KEY_FILES})
  set (provision_depends kernel_elf)
endif()
