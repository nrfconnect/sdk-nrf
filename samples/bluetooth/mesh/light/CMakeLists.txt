#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

include_directories(
	${NRF_DIR}/samples/bluetooth/mesh/common
)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/model_handler.c)
target_include_directories(app PRIVATE
	include
	${NRF_DIR}/samples/bluetooth/mesh/common
)

# Preinitialization related to DFU over SMP for nRF52 series
if(CONFIG_SOC_SERIES_NRF52X)
  target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE
    ${NRF_DIR}/samples/bluetooth/mesh/common/smp_bt.c)
endif()

# NORDIC SDK APP END
