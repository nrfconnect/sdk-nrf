#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hci_cmd_cb_host_buffer_size_test)

# Generate runner for the test
test_runner_generate(src/hci_cmd_cb_host_buffer_size_test.c)

# Add Unit Under Test source files
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/controller/hci_internal_wrappers.c)

# Add test source file
target_sources(app PRIVATE src/hci_cmd_cb_host_buffer_size_test.c)

# Create mocks
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/softdevice_controller/include/sdc_hci_cmd_controller_baseband.h)

# Include paths
include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/controller)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
