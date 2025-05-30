# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This script defines a CMake target 'generate_kmu_keyfile_json' to create keyfile.json
# using 'west ncs-provision upload --dry-run'.

# --- Define paths and executables ---
set(NCS_PROVISION_OUTPUT_JSON_FILE "${CMAKE_BINARY_DIR}/keyfile.json")
set(NCS_PROVISION_BUILD_DIR "${CMAKE_BINARY_DIR}") # Build directory for ncs-provision context

# --- Construct the list of commands and dependencies ---
set(kmu_json_commands "")
set(kmu_json_dependencies "")

# First command: Generate keyfile for BL_PUBKEY
if(SB_CONFIG_SECURE_BOOT_GENERATE_DEFAULT_KMU_KEYFILE)

    # --- Determine the signing key file to use ---
    set(SIGNATURE_PRIVATE_KEY_FILE "") # Initialize

    if(DEFINED SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE AND NOT "${SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE}" STREQUAL "")

        string(CONFIGURE "${SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE}" keyfile)
        if(IS_ABSOLUTE ${keyfile})
            set(SIGNATURE_PRIVATE_KEY_FILE ${keyfile})
        else()
            set(SIGNATURE_PRIVATE_KEY_FILE ${APPLICATION_CONFIG_DIR}/${keyfile})
        endif()
        set(keyfile)

        if(NOT EXISTS ${SIGNATURE_PRIVATE_KEY_FILE})
            message(FATAL_ERROR "Config points to non-existing PEM file '${SIGNATURE_PRIVATE_KEY_FILE}'")
        endif()
    else()
        set(SIGNATURE_PRIVATE_KEY_FILE "${CMAKE_BINARY_DIR}/GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem")
    endif()

    list(APPEND kmu_json_commands
        COMMAND ${Python3_EXECUTABLE} -m west ncs-provision upload
                --keyname BL_PUBKEY
                --key ${SIGNATURE_PRIVATE_KEY_FILE}
                --build-dir ${NCS_PROVISION_BUILD_DIR}
                --dry-run
    )
    list(APPEND kmu_json_dependencies ${SIGNATURE_PRIVATE_KEY_FILE})
endif()

# Second command (conditional): Update keyfile for UROT_PUBKEY
if(SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE)
    list(APPEND kmu_json_commands
        COMMAND ${Python3_EXECUTABLE} -m west ncs-provision upload
                --keyname UROT_PUBKEY
                --key ${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}
                --build-dir ${NCS_PROVISION_BUILD_DIR}
                --dry-run
    )
    list(APPEND kmu_json_dependencies ${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE})
endif()

# --- Add custom command to generate/update keyfile.json ---
if(NOT kmu_json_commands STREQUAL "")
    add_custom_command(
        OUTPUT ${NCS_PROVISION_OUTPUT_JSON_FILE}
        ${kmu_json_commands} # Expands to one or more COMMAND clauses
        DEPENDS ${kmu_json_dependencies}
        COMMENT "Generating/Updating KMU keyfile JSON (${NCS_PROVISION_OUTPUT_JSON_FILE})"
        VERBATIM
    )

    # --- Add custom target to trigger the generation ---
    add_custom_target(
        generate_kmu_keyfile_json ALL
        DEPENDS ${NCS_PROVISION_OUTPUT_JSON_FILE}
    )
endif()
