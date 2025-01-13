#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(flash_ipuc)

target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/suit/ipuc/include)
target_include_directories(app PRIVATE mocks)

zephyr_compile_definitions(CONFIG_FLASH_IPUC_COUNT=2)
zephyr_compile_definitions(CONFIG_FLASH_IPUC_LOG_LEVEL)
zephyr_compile_definitions(CONFIG_FLASH_IPUC_LOG_LEVEL_DBG)
# Pretend to call API as nRF54H20 application FW
zephyr_compile_definitions(CONFIG_SOC_NRF54H20_CPUAPP)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/flash/flash_ipuc/flash_ipuc.c
)
