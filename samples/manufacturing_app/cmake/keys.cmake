#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Build the manufacturing key blob (mfg_keys.bin / mfg_keys.hex) from the
# files in keys/ and merge mfg_keys.hex into a merged.hex flashed by the
# default runner. Layout lives in src/mfg_key_blob.h and
# scripts/pack_mfg_keys.py.

# ---------------------------------------------------------------------------
# Resolve the mfg_keys partition address and size from devicetree.
# ---------------------------------------------------------------------------
dt_nodelabel(MFG_KEYS_NODE NODELABEL "mfg_keys_partition" REQUIRED)
dt_reg_addr(MFG_KEYS_ADDR PATH "${MFG_KEYS_NODE}")
dt_reg_size(MFG_KEYS_SIZE PATH "${MFG_KEYS_NODE}")

# ---------------------------------------------------------------------------
# Inputs and outputs
# ---------------------------------------------------------------------------
set(MFG_KEYS_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/keys)
set(MFG_PACK_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack_mfg_keys.py)
set(MFG_KEYS_BIN    ${PROJECT_BINARY_DIR}/zephyr/mfg_keys.bin)
set(MFG_KEYS_HEX    ${PROJECT_BINARY_DIR}/zephyr/mfg_keys.hex)
set(MFG_MERGED_HEX  ${PROJECT_BINARY_DIR}/zephyr/merged.hex)

file(GLOB_RECURSE MFG_KEYS_INPUTS
  "${MFG_KEYS_DIR}/*.pem"
  "${MFG_KEYS_DIR}/*.bin"
  "${MFG_KEYS_DIR}/*.txt"
  "${MFG_KEYS_DIR}/key_verification_msgs/*.msg"
)

# ---------------------------------------------------------------------------
# Pack the keys/ directory into the mfg_keys blob (.bin + .hex).
# ---------------------------------------------------------------------------
add_custom_command(
  OUTPUT  ${MFG_KEYS_BIN} ${MFG_KEYS_HEX}
  COMMAND ${PYTHON_EXECUTABLE} ${MFG_PACK_SCRIPT}
          --keys-dir ${MFG_KEYS_DIR}
          --addr     ${MFG_KEYS_ADDR}
          --size     ${MFG_KEYS_SIZE}
          --out-bin  ${MFG_KEYS_BIN}
          --out-hex  ${MFG_KEYS_HEX}
  DEPENDS ${MFG_PACK_SCRIPT} ${MFG_KEYS_INPUTS}
  COMMENT "Packing mfg_keys blob (${MFG_KEYS_SIZE} B at ${MFG_KEYS_ADDR})"
)

add_custom_target(mfg_keys_artefact ALL DEPENDS ${MFG_KEYS_BIN} ${MFG_KEYS_HEX})
add_dependencies(app mfg_keys_artefact)

# ---------------------------------------------------------------------------
# Dev convenience: merge zephyr.hex + mfg_keys.hex into merged.hex and point
# runners.yaml at it so `west flash` programs both regions.
# Production users ignore merged.hex and flash zephyr.hex and mfg_keys.bin
# separately (see README).
# ---------------------------------------------------------------------------
add_custom_command(
  OUTPUT  ${MFG_MERGED_HEX}
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
          -o ${MFG_MERGED_HEX}
          ${BYPRODUCT_KERNEL_HEX_NAME}
          ${MFG_KEYS_HEX}
  DEPENDS ${BYPRODUCT_KERNEL_HEX_NAME}
          ${MFG_KEYS_HEX}
          ${logical_target_for_zephyr_elf}
  COMMENT "Merging zephyr.hex + mfg_keys.hex -> merged.hex"
)

set(MFG_RUNNERS_STAMP ${PROJECT_BINARY_DIR}/zephyr/.mfg_runners_patched)
add_custom_command(
  OUTPUT  ${MFG_RUNNERS_STAMP}
  COMMAND ${CMAKE_COMMAND}
          -DRUNNERS_YAML=${PROJECT_BINARY_DIR}/zephyr/runners.yaml
          -DMERGED_HEX=${MFG_MERGED_HEX}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_runners.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${MFG_RUNNERS_STAMP}
  DEPENDS ${MFG_MERGED_HEX} ${PROJECT_BINARY_DIR}/zephyr/runners.yaml
  COMMENT "Pointing runners.yaml hex_file at merged.hex"
)

add_custom_target(mfg_merged_hex ALL DEPENDS ${MFG_MERGED_HEX} ${MFG_RUNNERS_STAMP})
