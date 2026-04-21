# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ram_context_for_isr)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/application_development/ram_context_for_isr/src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(${ZEPHYR_BASE}/tests/application_development/ram_context_for_isr/include)

if(CONFIG_CPU_CORTEX_M_HAS_VTOR)
  zephyr_code_relocate(FILES ${ZEPHYR_BASE}/arch/arm/core/cortex_m/isr_wrapper.c LOCATION RAM)
  zephyr_code_relocate(FILES ${ZEPHYR_BASE}/arch/arm/core/cortex_m/exc_exit.c LOCATION RAM)
endif()

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/tests/application_development/ram_context_for_isr/src/main.c FILTER ".test_irq_callback" LOCATION RAM)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/tests/application_development/ram_context_for_isr/src/fake_driver.c FILTER ".fake_driver_isr" LOCATION RAM)

# Only needed because the fake driver is defined in tests folder
zephyr_linker_sources(SECTIONS ${ZEPHYR_BASE}/tests/application_development/ram_context_for_isr/sections-rom.ld)
