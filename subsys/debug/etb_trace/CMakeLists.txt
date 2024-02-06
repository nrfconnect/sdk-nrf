#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(etb_trace.c)
zephyr_library_sources_ifdef(CONFIG_ETB_TRACE_LOW_POWER etb_trace_lp.c)
zephyr_library_include_directories(.)
zephyr_library_include_directories(${ZEPHYR_BASE}/kernel/include)

# Include soc_cpu_idle.h globally only when it's used
zephyr_include_directories_ifdef(CONFIG_ARM_ON_EXIT_CPU_IDLE include)
