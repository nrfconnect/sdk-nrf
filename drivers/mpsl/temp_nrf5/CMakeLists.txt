#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_TEMP_NRF5_MPSL temp_nrf5_mpsl.c)

if(CONFIG_TEMP_NRF5 AND CONFIG_TEMP_NRF5_MPSL)
  message(FATAL_ERROR "CONFIG_TEMP_NRF5 and CONFIG_TEMP_NRF5_MPSL have both been enabled, but must not be used together.")
endif()
