# Kconfig - nRF91 PCA20035 board configuration
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

zephyr_library()

zephyr_library_sources_ifdef(
	CONFIG_TRUSTED_EXECUTION_NONSECURE
	board_nonsecure.c)

zephyr_library_sources_ifdef(
        CONFIG_TRUSTED_EXECUTION_SECURE
	board_secure.c)
