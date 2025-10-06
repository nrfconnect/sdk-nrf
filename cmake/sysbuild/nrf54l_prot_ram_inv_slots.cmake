# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This script defines a CMake target 'generate_prot_ram_inv_slots_json' to create prot_ram_inv_slots.json
# using 'west ncs-provision upload --dry-run'.

# --- Construct the list of commands and dependencies ---
set(kmu_json_commands "")
set(json_filename "prot_ram_inv_slots.json")

list(APPEND kmu_json_commands
  COMMAND ${Python3_EXECUTABLE} -m west ncs-provision upload
    --keyname PROT_RAM_INV_SLOTS
    --build-dir ${CMAKE_BINARY_DIR}
    --dry-run
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/${json_filename}
  ${kmu_json_commands} # Expands to one or more COMMAND clauses
  COMMENT "Generating/Updating KMU protected ram invalidation slots JSON (${CMAKE_BINARY_DIR}/${json_filename})"
  VERBATIM
)

# --- Add custom target to trigger the generation ---
add_custom_target(
  generate_prot_ram_inv_slot_json ALL
  DEPENDS ${CMAKE_BINARY_DIR}/${json_filename}
)
