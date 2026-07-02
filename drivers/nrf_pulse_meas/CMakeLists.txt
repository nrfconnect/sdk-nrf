#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#zephyr_syscall_header(${ZEPHYR_BASE}../nrf/include/zephyr/nrf_pulse_meas.h)

zephyr_library()

zephyr_sources_ifdef(CONFIG_NRF_PULSE_MEAS nrf_pulse_meas.c)
