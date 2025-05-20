#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(modem_slm.c)
zephyr_library_sources(modem_slm_monitor.c)
# SLM monitor's data must be in RAM
zephyr_linker_sources(RWDATA modem_slm_monitor.ld)
