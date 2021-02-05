#
# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_SOC_FLASH_NRF_LL_SOFTDEVICE flash)
add_subdirectory_ifdef(CONFIG_ENTROPY_NRF_LL_SOFTDEVICE entropy)
