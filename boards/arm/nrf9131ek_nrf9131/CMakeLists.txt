# Copyright (c) 2023 Nordic Semiconductor ASA.
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if (CONFIG_BOARD_NRF9131EK_NRF9131 OR CONFIG_BOARD_NRF9131EK_NRF9131_NS)
zephyr_library()
zephyr_library_sources(npm1300.c)
endif()
