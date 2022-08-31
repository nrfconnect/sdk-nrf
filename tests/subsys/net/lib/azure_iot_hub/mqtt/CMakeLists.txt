#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(azure_iot_hub_mqtt_test)

# Generate runner for the test
test_runner_generate(src/azure_iot_hub_mqtt_test.c)

# Create mock
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/mqtt.h)
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/socket.h net)

# Add Unit Under Test source files
target_sources(app PRIVATE
        ${NRF_DIR}/subsys/net/lib/azure_iot_hub/src/azure_iot_hub_mqtt.c
)

# Add test source file
target_sources(app PRIVATE src/azure_iot_hub_mqtt_test.c)

# Include paths
target_include_directories(app PRIVATE ${NRF_DIR}/include/zephyr/net/)
target_include_directories(app PRIVATE ${NRF_DIR}/subsys/net/lib/azure_iot_hub/include/)
target_include_directories(app PRIVATE ${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/inc/)

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
        -DCONFIG_MQTT_LIB_TLS=1
        -DCONFIG_NET_SOCKETS_POSIX_NAMES=1
        -DCONFIG_AZURE_IOT_HUB_HOSTNAME="test-hostname.azure-devices.net"
        -DCONFIG_AZURE_IOT_HUB_STATIC_IP_ADDRESS=""
        -DCONFIG_AZURE_IOT_HUB_PORT=8883
        -DCONFIG_AZURE_IOT_HUB_SEND_TIMEOUT_SEC=60
        -DCONFIG_AZURE_IOT_HUB_MQTT_RX_TX_BUFFER_LEN=256
        -DCONFIG_AZURE_IOT_HUB_MQTT_PAYLOAD_BUFFER_LEN=2304
        -DCONFIG_AZURE_IOT_HUB_STACK_SIZE=2560
        -DCONFIG_AZURE_IOT_HUB_SEC_TAG=1
        -DCONFIG_AZURE_IOT_HUB_SECONDARY_SEC_TAG=-1
)
