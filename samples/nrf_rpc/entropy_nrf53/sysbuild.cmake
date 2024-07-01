#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

# Include app core image if enabled
if(SB_CONFIG_SOC_NRF5340_CPUAPP)
  # Get application core board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  set(board_target_netcore "${BOARD}/${target_soc}/cpunet")
  set(target_soc)

  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${board_target_netcore}
    BOARD_REVISION ${BOARD_REVISION}
  )

  if(NOT "CPUNET" IN_LIST PM_DOMAINS)
    list(APPEND PM_DOMAINS CPUNET)
  endif()

  set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES "remote")
  set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET "remote")
  set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")
endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
