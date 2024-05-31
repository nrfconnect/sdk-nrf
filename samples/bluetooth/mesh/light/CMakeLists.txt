#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

include_directories(
	${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common
)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/model_handler.c)
target_include_directories(app PRIVATE
	include
	${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common
)

# Preinitialization related to DFU over SMP
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE
	${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common/smp_bt.c)

# NORDIC SDK APP END
