#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Include PPR core image if enabled
if(SB_CONFIG_SUPPORT_PPRCORE AND NOT SB_CONFIG_PPRCORE_NONE AND DEFINED SB_CONFIG_PPRCORE_IMAGE_NAME)
  # Calculate the PPR core board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)
  set(board_target_pprcore "${BOARD}/${target_soc}/${SB_CONFIG_PPRCORE_REMOTE_BOARD_TARGET_CPUCLUSTER}")
  set(target_soc)
  set(target_cpucluster)

  ExternalZephyrProject_Add(
    APPLICATION ${SB_CONFIG_PPRCORE_IMAGE_NAME}
    SOURCE_DIR ${SB_CONFIG_PPRCORE_IMAGE_PATH}
    BOARD ${board_target_pprcore}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
