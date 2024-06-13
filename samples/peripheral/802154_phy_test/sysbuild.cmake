#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

zephyr_get(EXTRA_ZEPHYR_MODULES)
set(EXTRA_ZEPHYR_MODULES "${EXTRA_ZEPHYR_MODULES};${CMAKE_CURRENT_LIST_DIR}/modules/app_rpc" CACHE INTERNAL "extra modules directories")

# Include app core image if enabled
if(SB_CONFIG_BOARD_NRF5340DK_NRF5340_CPUNET)
  ExternalZephyrProject_Add(
    APPLICATION remote_shell
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/remote_shell
    BOARD "nrf5340dk/nrf5340/cpuapp"
  )

  if(NOT "CPUAPP" IN_LIST PM_DOMAINS)
    list(APPEND PM_DOMAINS CPUAPP)
  endif()

  set_property(GLOBAL APPEND PROPERTY PM_CPUAPP_IMAGES "remote_shell")
  set_property(GLOBAL PROPERTY DOMAIN_APP_CPUAPP "remote_shell")
  set(CPUAPP_PM_DOMAIN_DYNAMIC_PARTITION remote_shell CACHE INTERNAL "")
endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
