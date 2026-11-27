#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dtm_twowire)

target_sources(app PRIVATE src/main.c)

# The SoftDevice Controller is not available on native_sim, so its HCI headers and
# the nrfx.h they depend on have to be provided to the library by the test.
if(CONFIG_DTM_TWOWIRE_TO_HCI_SDC_VS_COMMANDS)
  zephyr_include_directories(
    ${ZEPHYR_NRFXLIB_MODULE_DIR}/softdevice_controller/include
    ${ZEPHYR_NRF_MODULE_DIR}/tests/subsys/bluetooth/controller
  )
endif()
