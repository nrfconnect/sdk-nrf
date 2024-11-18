# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_SDP)
  # Extract SoC name from related variables
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  set(board_target_flpr "${BOARD}/${target_soc}/cpuflpr")
  set(target_soc)

  # Select the SDP application
  if(SB_CONFIG_SDP_GPIO)
    set(sdp_app_dir "${ZEPHYR_NRF_MODULE_DIR}/applications/sdp/gpio")
  elseif(SB_CONFIG_SDP_MSPI)
    set(sdp_app_dir "${ZEPHYR_NRF_MODULE_DIR}/applications/sdp/mspi")
  else()
    message(FATAL_ERROR "Unknown SDP application type")
  endif()

  # Include the SDP application in the build
  ExternalZephyrProject_Add(
    APPLICATION sdp
    SOURCE_DIR ${sdp_app_dir}
    BOARD ${board_target_flpr}
    BOARD_REVISION ${BOARD_REVISION}
  )
  set(sdp_app_dir)
  set(board_target_flpr)
endif()
