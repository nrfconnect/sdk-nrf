#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(json)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/net/lib/azure_iot_hub/src/azure_iot_hub_topic.c
)

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/net/lib/azure_iot_hub/include/
)

target_compile_options(app
  PRIVATE
  -DCONFIG_AZURE_IOT_HUB_TOPIC_ELEMENT_MAX_LEN=48
  -DCONFIG_AZURE_IOT_HUB_PROPERTY_BAG_MAX_COUNT=5
  -DCONFIG_AZURE_IOT_HUB_LOG_LEVEL=0
  -DCONFIG_AZURE_IOT_HUB_TOPIC_PROPERTY_BAG_PREFIX=y
)
