# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_SDP)
  # Extract SoC name from related variables
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  set(board_target_flpr "${BOARD}/${target_soc}/cpuflpr")
  set(target_soc)

  # Include the SDP application in the build
  ExternalZephyrProject_Add(
    APPLICATION sdp
    SOURCE_DIR ${SB_CONFIG_SDP_IMAGE_PATH}
    BOARD ${board_target_flpr}
    BOARD_REVISION ${BOARD_REVISION}
  )
  set(board_target_flpr)
endif()
