#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(st25r3911b_spi.c)
zephyr_library_sources(st25r3911b_common.c)
zephyr_library_sources(st25r3911b_interrupt.c)
zephyr_library_sources(st25r3911b_nfca.c)
