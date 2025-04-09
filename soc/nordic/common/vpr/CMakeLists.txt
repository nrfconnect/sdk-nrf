# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/soc/nordic/common/vpr)

zephyr_library_sources(
  ${ZEPHYR_BASE}/soc/nordic/common/vpr/soc_context.S
  ${ZEPHYR_BASE}/soc/nordic/common/vpr/soc_idle.c
  ${ZEPHYR_BASE}/soc/nordic/common/vpr/soc_init.c
  )

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
