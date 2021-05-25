#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
if(CONFIG_SLM_NRF52_DFU)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/slm_at_dfu.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu_target_nrf52.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu_host.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/slip.c)
endif()
