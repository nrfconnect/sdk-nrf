# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_sources(board.c)

zephyr_compile_definitions_ifdef(CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP_EMU NRFX_COREDEP_DELAY_US_LOOP_CYCLES=6)
zephyr_compile_definitions_ifdef(CONFIG_BOARD_NRF7120PDK_NRF7120_CPUAPP NRFX_COREDEP_DELAY_US_LOOP_CYCLES=6)
