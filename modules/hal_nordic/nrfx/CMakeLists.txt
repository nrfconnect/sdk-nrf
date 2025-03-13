# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

# Inherit key properties from the upstream nrfx library.
zephyr_library_get_current_dir_lib_name(${ZEPHYR_NRF_MODULE_DIR} zephyr_lib_nrfx)
zephyr_library_include_directories($<TARGET_PROPERTY:${zephyr_lib_nrfx},INCLUDE_DIRECTORIES>)
zephyr_library_compile_definitions($<TARGET_PROPERTY:${zephyr_lib_nrfx},COMPILE_DEFINITIONS>)


# Definitions copied from Zephyr.
set(INC_DIR ${NRFX_DIR}/drivers/include)
set(SRC_DIR ${NRFX_DIR}/drivers/src)
set(MDK_DIR ${NRFX_DIR}/mdk)
set(HELPERS_DIR ${NRFX_DIR}/helpers)

macro(mdk_svd_ifdef feature_toggle filename)
  if(${feature_toggle})
    set(SOC_SVD_FILE ${MDK_DIR}/${filename} CACHE FILEPATH "Path to a CMSIS-SVD file")
  endif()
endmacro()


# Emulator-specific definitions.
zephyr_compile_definitions_ifdef(CONFIG_EMULATOR_FPGA    BOARD_FPGA)
zephyr_compile_definitions_ifdef(CONFIG_EMULATOR_SYSTEMC BOARD_SYSTEMC)
zephyr_compile_definitions_ifdef(CONFIG_EMULATOR_Z1      BOARD_PALLADIUM)

# Add definitions for private CPUs - NOT to be upstreamed.


# Add definitions for products which are yet to be upstreamed.

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA                NRF7120_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUAPP         NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUFLPR        NRF_FLPR)
zephyr_compile_definitions_ifdef(CONFIG_SOC_COMPATIBLE_NRF7120_ENGA     NRF7120_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_COMPATIBLE_NRF7120_ENGA_CPUAPP NRF_APPLICATION)

zephyr_library_sources_ifdef(CONFIG_SOC_NRF7120_ENGA        ${MDK_DIR}/system_nrf7120_enga.c)

mdk_svd_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUAPP      nrf7120_enga_application.svd)
mdk_svd_ifdef(CONFIG_SOC_NRF7120_ENGA_CPUFLPR     nrf7120_enga_flpr.svd)

if(CONFIG_SOC_NRF7120_ENGA_CPUAPP)
  dt_prop(clock_frequency PATH "/cpus/cpu@0" PROPERTY "clock-frequency")
  math(EXPR clock_frequency_mhz "${clock_frequency} / 1000000")
  zephyr_compile_definitions("NRF_CONFIG_CPU_FREQ_MHZ=${clock_frequency_mhz}")
endif()
