#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
list(GET split_board_qualifiers 1 target_soc)
list(GET split_board_qualifiers 2 target_cpucluster)
set(board_target_radiocore "${BOARD}/${target_soc}/${SB_CONFIG_RADIO_BOARD_TARGET_CPUCLUSTER}")
set(target_soc)
set(target_cpucluster)

if(NOT "${SB_CONFIG_RADIO_BOARD_TARGET_CPUCLUSTER}" STREQUAL "")
  ExternalZephyrProject_Add(
    APPLICATION rad_image
    SOURCE_DIR ${APP_DIR}/rad_image
    BOARD ${board_target_radiocore}
    BOARD_REVISION ${BOARD_REVISION}
  )

  add_dependencies(ssf_client rad_image)
  sysbuild_add_dependencies(FLASH ssf_client rad_image)
endif()
