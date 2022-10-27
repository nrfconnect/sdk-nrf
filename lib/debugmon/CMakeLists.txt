#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(
    JLINK_MONITOR.c
    JLINK_MONITOR_ISR_SES.s
)

zephyr_include_directories(include)
