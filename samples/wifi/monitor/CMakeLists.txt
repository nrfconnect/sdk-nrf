#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_monitor)

target_include_directories_ifdef(CONFIG_WIFI_SAMPLE_USBD
  app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/samples/wifi/common/src)

target_sources_ifdef(CONFIG_WIFI_SAMPLE_USBD
  app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/samples/wifi/common/src/wifi_sample_usbd.c)
target_sources(app PRIVATE src/main.c)
