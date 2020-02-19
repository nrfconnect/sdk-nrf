#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

# This sample runs as a non-secure application on nRF91. Therefore, it
# requires the secure_partition_manager that prepares the required
# peripherals to be available for the application.
#
# Configure the SPM image to enable the peripherals that this sample
# needs.
set(spm_CONF_FILE
  prj.conf
  ${CMAKE_CURRENT_LIST_DIR}/child_secure_partition_manager.conf
  )
include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(serial_lte_modem)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/slm_at_host.c)
target_sources(app PRIVATE src/slm_at_tcpip.c)
target_sources(app PRIVATE src/slm_at_icmp.c)
target_sources(app PRIVATE src/slm_at_gps.c)

zephyr_include_directories(src)
