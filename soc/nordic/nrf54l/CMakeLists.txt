# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_sources(
  ${ZEPHYR_BASE}/soc/nordic/nrf54l/soc.c
  )
zephyr_include_directories(${ZEPHYR_BASE}/soc/nordic/nrf54l)
add_subdirectory(${ZEPHYR_BASE}/soc/nordic/common ${CMAKE_BINARY_DIR}/common)

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LM20A_ENGA NRF_SKIP_TAMPC_SETUP)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LM20A_ENGA NRF54LM20A_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LM20A_ENGA_CPUAPP  NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LM20A_ENGA_CPUFLPR NRF_FLPR)

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA NRF_SKIP_TAMPC_SETUP)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA NRF54LV10A_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP  NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUFLPR NRF_FLPR)
