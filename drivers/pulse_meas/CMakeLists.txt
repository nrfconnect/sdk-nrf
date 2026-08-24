#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_sources_ifdef(CONFIG_PULSE_MEAS pulse_meas.c)

zephyr_syscall_header(${ZEPHYR_NRF_MODULE_DIR}/include/drivers/pulse_meas.h)
