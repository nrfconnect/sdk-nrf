# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# If it is enabled, include the SDP GPIO FLPR application in the build
if(SB_CONFIG_EGPIO_FLPR_APPLICATION)
  # Extract SoC name from related variables
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  set(board_target_flpr "${BOARD}/${target_soc}/cpuflpr")
  set(target_soc)

  ExternalZephyrProject_Add(
    APPLICATION flpr_egpio
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/applications/sdp/gpio
    BOARD ${board_target_flpr}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
