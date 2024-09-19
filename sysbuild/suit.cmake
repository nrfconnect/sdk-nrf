#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SUIT_BUILD_FLASH_COMPANION)
  # Calculate the network board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)

  set(board_target "${BOARD}/${target_soc}/${SB_CONFIG_FLASH_COMPANION_TARGET_CPUCLUSTER}")

  ExternalZephyrProject_Add(
    APPLICATION flash_companion
    SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/samples/suit/flash_companion"
    BOARD ${board_target}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()

if(SB_CONFIG_SUIT_BUILD_RECOVERY)
  # Calculate the network board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  list(GET split_board_qualifiers 2 target_cpucluster)

  set(board_target "${BOARD}/${target_soc}/${target_cpucluster}")

  ExternalZephyrProject_Add(
    APPLICATION recovery
    SOURCE_DIR ${SB_CONFIG_SUIT_RECOVERY_APPLICATION_PATH}
    BOARD ${board_target}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
