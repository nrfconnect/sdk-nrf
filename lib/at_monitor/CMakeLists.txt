#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(at_monitor.c)
# AT monitors data must be in RAM
zephyr_linker_sources(RWDATA at_monitor.ld)
