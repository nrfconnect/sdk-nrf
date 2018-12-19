#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(ARM ${ZEPHYR_BASE}/arch/arm/core)
set(CORTEX_M ${ARM}/cortex_m)
set(EXTS ${ZEPHYR_BASE}/ext)
set(HAL ${EXTS}/hal)

add_library(c_runtime_setup STATIC ${ZEPHYR_BASE}/misc/empty_file.c)
target_link_libraries(c_runtime_setup PRIVATE zephyr_interface)
# Make sure every generated file is present before trying to build the
# library.
target_link_libraries(c_runtime_setup PRIVATE bootloader)

# Set start symbol of MDK to point to the main_bl function
if(CONFIG_SB_C_RUNTIME_SETUP_VARIANT_MDK)
  add_definitions(-D__START=main_bl)
  add_definitions(-D__STARTUP_CLEAR_BSS)
endif()

target_sources_ifdef(CONFIG_SB_C_RUNTIME_SETUP_VARIANT_CUSTOM
  c_runtime_setup
  PRIVATE
  startup.c
  )

target_sources_ifdef(CONFIG_CPU_CORTEX_M0
  c_runtime_setup
  PUBLIC
  ${ARM}/irq_relay.S
  )

target_sources_ifdef(CONFIG_SB_C_RUNTIME_SETUP_VARIANT_ZEPHYR
  c_runtime_setup
  PRIVATE
  ${CORTEX_M}/prep_c.c
  ${CORTEX_M}/reset.S
  ${ARM}/irq_init.c
  ${CORTEX_M}/vector_table.S
  zephyr_dummy_interrupt_handlers.c
  )

target_include_directories_ifdef(CONFIG_SB_C_RUNTIME_SETUP_VARIANT_ZEPHYR
  c_runtime_setup
  PRIVATE
  ${CORTEX_M}
  ${CORTEX_M}/mpu
  ${ZEPHYR_BASE}/include
  ${ARM_INC}/include/cortex_m
  )

if(CONFIG_SB_C_RUNTIME_SETUP_VARIANT_ZEPHYR)
  add_library(kernel_init STATIC ${ZEPHYR_BASE}/kernel/init.c)
  target_link_libraries(kernel_init zephyr_interface)
  target_compile_definitions(kernel_init PRIVATE _Cstart=UNUSED_Cstart)
  add_dependencies(kernel_init offsets_h)
  target_link_libraries(c_runtime_setup PRIVATE kernel_init)
endif()

# Nordic specific for startup code
if(CONFIG_SOC_SERIES_NRF91X)
target_sources_ifdef(CONFIG_SB_VENDOR_SYSTEM_INIT
  c_runtime_setup
  PRIVATE
  ${HAL}/nordic/nrfx/mdk/system_nrf9160.c
  )
target_sources_ifdef(CONFIG_SB_C_RUNTIME_SETUP_VARIANT_MDK
  c_runtime_setup
  PRIVATE
  gcc_startup_nrf9160.S
  )
elseif(CONFIG_SOC_SERIES_NRF52X)
target_sources_ifdef(CONFIG_SB_VENDOR_SYSTEM_INIT
  c_runtime_setup
  PRIVATE
  ${HAL}/nordic/nrfx/mdk/system_nrf52840.c
  )
target_sources_ifdef(CONFIG_SB_C_RUNTIME_SETUP_VARIANT_MDK
  c_runtime_setup
  PRIVATE
  gcc_startup_nrf52840.S
  )
endif()
