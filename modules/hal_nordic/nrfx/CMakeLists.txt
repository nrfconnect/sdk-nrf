# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

set(ZEPHYR_CURRENT_LIBRARY "nrfx")

set(BSP_DIR ${NRFX_DIR}/bsp)
set(SOC_DIR ${BSP_DIR}/${CONFIG_SOC_NORDIC_BSP_NAME})
set(MDK_DIR ${SOC_DIR}/mdk)
set(mdk_dir ${NRFX_DIR}/mdk)
set(helpers_dir ${NRFX_DIR}/helpers)

# Add definitions for products which are yet to be upstreamed.
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LS05B_ENGA NRF54LS05B_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LS05B_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA NRF54LV10A_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUFLPR NRF_FLPR)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA NRF7120_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUAPP NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUFLPR NRF_FLPR)

zephyr_library_sources_ifdef(CONFIG_SOC_NRF7120_ENGA ${MDK_DIR}/system_nrf7120_enga.c)

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

if(CONFIG_NRFX_GPPI AND NOT CONFIG_NRFX_GPPI_V1 AND CONFIG_SOC_SERIES_NRF71X)
  zephyr_library_sources_ifdef(CONFIG_HAS_HW_NRF_PPI ${helpers_dir}/nrfx_gppi_ppi.c)
  zephyr_library_sources(${BSP_DIR}/stable/soc/interconnect/nrfx_gppi_lumos.c)
  if(CONFIG_HAS_HW_NRF_DPPIC)
    zephyr_library_sources(${helpers_dir}/nrfx_gppi_dppi.c)
  endif()
endif()
