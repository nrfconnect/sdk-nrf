#
# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_CLOCK_CONTROL_MPSL clock_control)
add_subdirectory_ifdef(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL flash_sync)
