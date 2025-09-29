# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_amend(${ZEPHYR_NRF_MODULE_DIR})

set(BSP_DIR ${NRFX_DIR}/bsp)

if(CONFIG_SOC_NORDIC_BSP_NAME STREQUAL "stable")
  set(SOC_DIR ${BSP_DIR}/${CONFIG_SOC_NORDIC_BSP_NAME})
else()
  string(TOUPPER ${CONFIG_SOC_NORDIC_BSP_NAME} BSP_MODULE_NAME)
  set(SOC_DIR "${ZEPHYR_${BSP_MODULE_NAME}_MODULE_DIR}")
endif()

set(MDK_DIR ${SOC_DIR}/mdk)

# Add definitions for products which are yet to be upstreamed.
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LM20A_ENGA NRF54LM20A_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LM20A_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LM20A_ENGA_CPUFLPR NRF_FLPR)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LS05B_ENGA NRF54LS05B_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LS05B_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA NRF54LV10A_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUFLPR NRF_FLPR)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA NRF7120_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUFLPR NRF_FLPR)

zephyr_library_sources_ifdef(CONFIG_SOC_NRF7120_ENGA ${MDK_DIR}/system_nrf7120_enga.c)

mdk_svd_ifdef(CONFIG_SOC_NRF54LM20A_ENGA_CPUAPP nrf54lm20a_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LM20A_ENGA_CPUFLPR nrf54lm20a_enga_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LS05B_ENGA_CPUAPP nrf54ls05b_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP nrf54lv10a_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUFLPR nrf54lv10a_enga_flpr.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUAPP nrf7120_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUFLPR nrf7120_enga_flpr.svd)

if(CONFIG_SOC_NRF7120_ENGA_CPUAPP)
  dt_prop(clock_frequency PATH "/cpus/cpu@0" PROPERTY "clock-frequency")
  math(EXPR clock_frequency_mhz "${clock_frequency} / 1000000")
  zephyr_compile_definitions("NRF_CONFIG_CPU_FREQ_MHZ=${clock_frequency_mhz}")
endif()

if(CONFIG_SOC_NRF54LS05B_ENGA_CPUAPP)
  dt_prop(clock_frequency PATH "/clocks/hfpll" PROPERTY "clock-frequency")
  math(EXPR clock_frequency_mhz "${clock_frequency} / 1000000")
  zephyr_compile_definitions("NRF_CONFIG_CPU_FREQ_MHZ=${clock_frequency_mhz}")
endif()
