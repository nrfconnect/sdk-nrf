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

mdk_svd_ifdef(CONFIG_SOC_NRF54LS05B_ENGA_CPUAPP nrf54ls05b_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP nrf54lv10a_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUFLPR nrf54lv10a_enga_flpr.svd)
