#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(ARM ${ZEPHYR_BASE}/arch/arm/core)
set(CORTEX_M ${ARM}/cortex_m)
set(EXTS ${ZEPHYR_BASE}/ext)
set(HAL ${EXTS}/hal)

zephyr_library()

# TODO Use zephyr build code for this directly?
zephyr_library_sources(
  ${CORTEX_M}/prep_c.c
  ${CORTEX_M}/reset.S
  ${ARM}/irq_init.c
  ${ARM}/exc_exit.S
  ${CORTEX_M}/vector_table.S
  ${ZEPHYR_BASE}/drivers/timer/cortex_m_systick.c
  zephyr_dummy_interrupt_handlers.c
  )
zephyr_library_link_libraries(${IMAGE}kernel_init)

zephyr_library_named(kernel_init)
zephyr_library_sources(${ZEPHYR_BASE}/kernel/init.c)
zephyr_library_compile_definitions(z_cstart=UNUSED_z_cstart)
add_dependencies(${IMAGE}kernel_init ${IMAGE}offsets_h)
