#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This sample always requires PPR snippet for nRF54h20.
if (${NORMALIZED_BOARD_TARGET} STREQUAL "nrf54h20dk_nrf54h20_cpuapp")
  if(NOT nordic-ppr IN_LIST event_manager_proxy_SNIPPET)
    set(event_manager_proxy_SNIPPET nordic-ppr CACHE STRING "" FORCE)
  endif()
endif()

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
)
set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES remote)
set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET remote)
set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")

# Add a dependency so that the remote sample will be built and flashed first
sysbuild_add_dependencies(CONFIGURE event_manager_proxy remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH event_manager_proxy remote)
