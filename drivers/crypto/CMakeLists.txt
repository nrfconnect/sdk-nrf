#
# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_CRYPTO_NRF_ECB crypto_nrf_ecb.c)
