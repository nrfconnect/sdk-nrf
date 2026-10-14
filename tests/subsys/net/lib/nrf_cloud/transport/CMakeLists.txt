#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_transport_test)

set(NRF_CLOUD_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud)

# The transport is compiled alone, its dependencies are faked
target_sources(app PRIVATE
  src/main.c
  ${NRF_CLOUD_DIR}/mqtt/src/nrf_cloud_transport.c
)
target_include_directories(app PRIVATE
  ${NRF_CLOUD_DIR}/common/include
  ${NRF_CLOUD_DIR}/mqtt/include
)
target_compile_definitions(app PRIVATE
  CONFIG_NRF_CLOUD_LOG_LEVEL=4
  CONFIG_NRF_CLOUD_HOST_NAME="mqtt.nrfcloud.com"
  CONFIG_NRF_CLOUD_PORT=8883
  CONFIG_NRF_CLOUD_MQTT_KEEPALIVE=1200
  CONFIG_NRF_CLOUD_MQTT_MESSAGE_BUFFER_LEN=256
  CONFIG_NRF_CLOUD_MQTT_PAYLOAD_BUFFER_LEN=256
  CONFIG_NRF_CLOUD_CLIENT_ID_SRC_RUNTIME=1
  CONFIG_NRF_CLOUD_SEND_TIMEOUT_SEC=60
  CONFIG_MQTT_LIB_TLS=1
  CONFIG_MQTT_CLEAN_SESSION=1
  CONFIG_CONNECT_BY_NAME_MAX_ADDRS=4
)
