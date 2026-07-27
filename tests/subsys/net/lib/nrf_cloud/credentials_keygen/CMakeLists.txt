#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_credentials_keygen_test)

target_sources(app PRIVATE src/main.c)

# Compile the units under test directly instead of pulling in the whole
# nRF Cloud library. Their external nRF Cloud dependencies
# (nrf_cloud_sec_tag_get/nrf_cloud_client_id_get) are provided as fakes in
# src/main.c; everything else (PSA crypto, mbedTLS, TLS credentials) is linked
# for real.
target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_credentials_keygen.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_credentials_shell.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/include
)

target_compile_options(app
  PRIVATE
  -DCONFIG_NRF_CLOUD_LOG_LEVEL=4
  # The units under test are gated by these Kconfigs in the real build. The
  # library Kconfig is not reachable here (the library is not enabled), so
  # define them for the direct compilation instead.
  -DCONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN=1
  -DCONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_SHELL=1
  -DCONFIG_NRF_CLOUD_CREDENTIALS_KEYGEN_VERIFY=1
)
