#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

# Include net core provision image for 5340
if(SB_CONFIG_SOC_NRF5340_CPUAPP)
  # Get network core board target
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)
  set(board_target_netcore "${BOARD}/${target_soc}/cpunet")
  set(target_soc)

  ExternalZephyrProject_Add(
    APPLICATION provisioning_image_net_core
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/tfm/provisioning_image_net_core
    BOARD ${board_target_netcore}
    BOARD_REVISION ${BOARD_REVISION}
  )

  if(NOT "CPUNET" IN_LIST PM_DOMAINS)
    list(APPEND PM_DOMAINS CPUNET)
  endif()

  set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES "provisioning_image_net_core")
  set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET "provisioning_image_net_core")
  set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION provisioning_image_net_core CACHE INTERNAL "")
endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
