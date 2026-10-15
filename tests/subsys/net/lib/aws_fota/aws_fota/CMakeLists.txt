#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aws_fota_test)

test_runner_generate(src/aws_fota_test.c)

# Create mocks
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/mqtt.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/fota_download.h)

# The unit under test is built by CONFIG_AWS_FOTA.
target_sources(app
  PRIVATE
  src/aws_fota_test.c
)

# The fota_download library is mocked, so the real one must not be built.
set_source_files_properties(
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/src/fota_download.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/src/util/fota_download_util.c
  DIRECTORY ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download
  PROPERTIES HEADER_FILE_ONLY ON
)
