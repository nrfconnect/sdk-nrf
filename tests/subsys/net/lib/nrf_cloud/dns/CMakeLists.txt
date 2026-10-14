#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_dns_test)

set(NRF_CLOUD_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud)

target_sources(app PRIVATE
  src/main.c
  ${NRF_CLOUD_DIR}/common/src/nrf_cloud_dns.c
)
target_include_directories(app PRIVATE ${NRF_CLOUD_DIR}/common/include)
target_compile_definitions(app PRIVATE
  CONFIG_NRF_CLOUD_LOG_LEVEL=4
)

if(CONFIG_TEST_NRF_CLOUD_IPV6)
  target_compile_definitions(app PRIVATE CONFIG_NRF_CLOUD_IPV6=1)
endif()

if(CONFIG_TEST_NRF_CLOUD_STATIC_IPV4)
  target_compile_definitions(app PRIVATE
    CONFIG_NRF_CLOUD_STATIC_IPV4=1
    CONFIG_NRF_CLOUD_STATIC_IPV4_ADDR="192.0.2.1"
  )
endif()
