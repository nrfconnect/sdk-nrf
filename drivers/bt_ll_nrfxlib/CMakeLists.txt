#
# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_SOC_FLASH_NRF_LL_NRFXLIB flash)
add_subdirectory_ifdef(CONFIG_ENTROPY_NRF_LL_NRFXLIB entropy)
