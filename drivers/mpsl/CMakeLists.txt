#
# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_CLOCK_CONTROL_MPSL clock_control)
add_subdirectory(flash_sync)
add_subdirectory_ifdef(CONFIG_TEMP_NRF5_MPSL temp_nrf5)
