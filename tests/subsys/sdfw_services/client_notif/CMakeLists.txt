#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ssf_client_notif_test)

set(SSF_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/sdfw_services)

# generate runner for the test
test_runner_generate(src/ssf_client_notif_test.c)

# create mocks
cmock_handle(${SSF_DIR}/transport/include/ssf_client_transport.h)

# add test file
target_sources(app PRIVATE src/ssf_client_notif_test.c)

# add file under test
target_sources(app
  PRIVATE
  ${SSF_DIR}/ssf_client.c
  ${SSF_DIR}/ssf_client_notif.c
  ${SSF_DIR}/os/ssf_client_zephyr.c
)

target_include_directories(app
  PRIVATE
  ${SSF_DIR}/os
)

target_compile_options(app
  PRIVATE
  -DCONFIG_SSF_CLIENT_DOMAIN_ID=2
  -DCONFIG_SSF_CLIENT_LOG_LEVEL=2
  -DCONFIG_SSF_CLIENT_REGISTERED_LISTENERS_MAX=2
  -DCONFIG_SSF_CLIENT_ZCBOR_MAX_BACKUPS=1
)
