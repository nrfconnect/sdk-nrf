# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This script defines a CMake target 'generate_kmu_keyfile_json' to create keyfile.json
# using 'west ncs-provision upload --dry-run'.

# --- Construct the list of commands and dependencies ---
set(kmu_json_commands "")
set(kmu_json_dependencies "")

# First command: Generate keyfile for BL_PUBKEY
if(SB_CONFIG_SECURE_BOOT_GENERATE_DEFAULT_KMU_KEYFILE)
  # --- Determine the signing key file to use ---
  set(signature_private_key_file "") # Initialize

  if(SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE)
    string(CONFIGURE "${SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE}" keyfile)
    if(IS_ABSOLUTE ${keyfile})
      set(signature_private_key_file ${keyfile})
    else()
      set(signature_private_key_file ${APPLICATION_CONFIG_DIR}/${keyfile})
    endif()
    set(keyfile)

    if(NOT EXISTS ${signature_private_key_file})
      message(FATAL_ERROR "Config points to non-existing PEM file '${signature_private_key_file}'")
    endif()
  else()
    set(signature_private_key_file "${CMAKE_BINARY_DIR}/GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem")
  endif()

  list(APPEND kmu_json_commands
    COMMAND ${Python3_EXECUTABLE} -m west ncs-provision upload
      --keyname BL_PUBKEY
      --key ${signature_private_key_file}
      --build-dir ${CMAKE_BINARY_DIR}
      --dry-run
  )
  list(APPEND kmu_json_dependencies ${signature_private_key_file})
endif()

# Second command (conditional): Update keyfile for UROT_PUBKEY
if(SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE)
  string(CONFIGURE "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}" mcuboot_signature_key_file)
  list(APPEND kmu_json_commands
    COMMAND ${Python3_EXECUTABLE} -m west ncs-provision upload
      --keyname UROT_PUBKEY
      --key ${mcuboot_signature_key_file}
      --build-dir ${CMAKE_BINARY_DIR}
      --dry-run
  )
  list(APPEND kmu_json_dependencies ${mcuboot_signature_key_file})
endif()

# --- Add custom command to generate/update keyfile.json ---
if(NOT kmu_json_commands STREQUAL "")
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/keyfile.json
    ${kmu_json_commands} # Expands to one or more COMMAND clauses
    DEPENDS ${kmu_json_dependencies}
    COMMENT "Generating/Updating KMU keyfile JSON (${CMAKE_BINARY_DIR}/keyfile.json)"
    VERBATIM
  )

  # --- Add custom target to trigger the generation ---
  add_custom_target(
    generate_kmu_keyfile_json ALL
    DEPENDS ${CMAKE_BINARY_DIR}/keyfile.json
  )
endif()
