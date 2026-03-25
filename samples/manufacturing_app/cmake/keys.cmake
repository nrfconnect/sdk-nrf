#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# keys.cmake
#
# Converts PEM public-key files, raw binary key files, and binary
# verification-message files from the keys/ directory into C byte-array
# headers that are compiled into the application.
#
# Outputs (placed in ${CMAKE_BINARY_DIR}/generated/):
#   provisioned_keys.h   - public-key byte arrays + binary key arrays + metadata
#   image_digests.h      - expected SHA-256 digests for BL1 / BL2 / app candidate
#
# Binary key files (build-time injected, replace with per-device data):
#   keys/ikg_seed.bin          - 48-byte IKG seed (Step 8)
#   keys/keyram_random0.bin    - 16-byte KeyRAM random slot 248 (Step 7)
#   keys/keyram_random1.bin    - 16-byte KeyRAM random slot 249 (Step 7)
#
# To provide per-device data, generate the binary files externally and pass
# them at build time using -DMFG_IKG_SEED_BIN=, -DMFG_KEYRAM_RANDOM0_BIN=,
# -DMFG_KEYRAM_RANDOM1_BIN= to point to the per-device files, then copy them
# into keys/ before invoking cmake, or use a west build overlay.
#

set(MFG_KEYS_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/keys)
set(MFG_GEN_DIR    ${CMAKE_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${MFG_GEN_DIR})

# ---------------------------------------------------------------------------
# Collect input files
# ---------------------------------------------------------------------------
file(GLOB MFG_PEM_FILES "${MFG_KEYS_DIR}/*.pem")
file(GLOB MFG_BIN_FILES "${MFG_KEYS_DIR}/*.bin")
file(GLOB MFG_MSG_FILES "${MFG_KEYS_DIR}/key_verification_msgs/*.msg")

# ---------------------------------------------------------------------------
# Run the Python conversion script at configure time to produce the header.
# Re-runs whenever a key, binary, or message file changes.
# ---------------------------------------------------------------------------
add_custom_command(
  OUTPUT  ${MFG_GEN_DIR}/provisioned_keys.h
  COMMAND ${PYTHON_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pem_to_c.py
          --keys-dir   ${MFG_KEYS_DIR}
          --msgs-dir   ${MFG_KEYS_DIR}/key_verification_msgs
          --output     ${MFG_GEN_DIR}/provisioned_keys.h
  DEPENDS ${MFG_PEM_FILES} ${MFG_BIN_FILES} ${MFG_MSG_FILES}
          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pem_to_c.py
  COMMENT "Generating provisioned_keys.h from PEM and binary files"
)

# ---------------------------------------------------------------------------
# Image digests: passed as CMake -D variables at build time, e.g.:
#   west build -- -DMFG_BL1_DIGEST=aabbcc...
# Defaults to all-zeros; a build with zeros will log a warning at runtime.
# ---------------------------------------------------------------------------
if(NOT DEFINED MFG_BL1_DIGEST)
  set(MFG_BL1_DIGEST "0000000000000000000000000000000000000000000000000000000000000000")
endif()
if(NOT DEFINED MFG_BL2A_DIGEST)
  set(MFG_BL2A_DIGEST "0000000000000000000000000000000000000000000000000000000000000000")
endif()
if(NOT DEFINED MFG_BL2B_DIGEST)
  set(MFG_BL2B_DIGEST "0000000000000000000000000000000000000000000000000000000000000000")
endif()
if(NOT DEFINED MFG_APP_CANDIDATE_DIGEST)
  set(MFG_APP_CANDIDATE_DIGEST "0000000000000000000000000000000000000000000000000000000000000000")
endif()

# Convert hex strings to C initializer lists (pairs of hex bytes)
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " BL1_BYTES   ${MFG_BL1_DIGEST})
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " BL2A_BYTES  ${MFG_BL2A_DIGEST})
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " BL2B_BYTES  ${MFG_BL2B_DIGEST})
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " APPC_BYTES  ${MFG_APP_CANDIDATE_DIGEST})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/image_digests.h.in
  ${MFG_GEN_DIR}/image_digests.h
  @ONLY
)

add_custom_target(mfg_generate_keys DEPENDS ${MFG_GEN_DIR}/provisioned_keys.h)
add_dependencies(app mfg_generate_keys)
