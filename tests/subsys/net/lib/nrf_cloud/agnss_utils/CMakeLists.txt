#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_agnss_utils_test)

set(nrfxlib_modem_dir ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem)
zephyr_include_directories(${nrfxlib_modem_dir}/include)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_agnss_utils.c
)

target_include_directories(app PRIVATE
  src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src
  ${ZEPHYR_BASE}/subsys/testsuite/include
)
