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

# Add Unit Under Test source files. The aws_jobs library is built by CONFIG_AWS_JOBS.
target_sources(app
  PRIVATE
  src/aws_fota_test.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/aws_fota/src/aws_fota.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/aws_fota/src/aws_fota_json.c
  ${ZEPHYR_BASE}/subsys/net/lib/http/http_parser_url.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/aws_fota/include/
  ${ZEPHYR_NRF_MODULE_DIR}/include/net/
  ${ZEPHYR_BASE}/include/zephyr/net/http/
  )

# Options of libraries that are not built as Zephyr libraries in this test.
target_compile_options(app
  PRIVATE
  -DCONFIG_AWS_FOTA_LOG_LEVEL=0
  -DCONFIG_AWS_FOTA_PAYLOAD_SIZE=512
  -DCONFIG_AWS_FOTA_DOWNLOAD_SECURITY_TAG=-1
  -DCONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE=256
  -DCONFIG_DOWNLOADER_MAX_FILENAME_SIZE=1024
  -DCONFIG_DOWNLOADER_TRANSPORT_PARAMS_SIZE=256
  -DCONFIG_DOWNLOADER_STACK_SIZE=1280
  )
