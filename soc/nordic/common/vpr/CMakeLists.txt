# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_include_directories(${ZEPHYR_BASE}/soc/nordic/common/vpr)

zephyr_library_sources(soc_context.S soc_idle.c soc_init.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
