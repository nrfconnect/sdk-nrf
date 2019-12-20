#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(json)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/net/lib/aws_fota/src/aws_fota_json.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/net/lib/aws_fota/include/
  )

# Do this in a non-standard way as the Kconfig options of "aws_jobs/Kconfig"
# is not executed. Hence these can not be set through prj.conf.
target_compile_options(app
  PRIVATE
  -DCONFIG_AWS_FOTA_HOSTNAME_MAX_LEN=1024
  -DCONFIG_AWS_FOTA_FILE_PATH_MAX_LEN=1024
  )
