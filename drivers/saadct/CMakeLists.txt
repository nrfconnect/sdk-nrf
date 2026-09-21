#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(saadct_nrfx.c)

zephyr_syscall_header(${ZEPHYR_NRF_MODULE_DIR}/include/drivers/saadct.h)
