#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mqtt_helper_test)

# Generate runner for the test
test_runner_generate(src/mqtt_helper_test.c)

# Create mock
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/mqtt.h)
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/socket.h zephyr/net)

# Add Unit Under Test source files
target_sources(app PRIVATE
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/mqtt_helper/mqtt_helper.c
)

# Add test source file
target_sources(app PRIVATE src/mqtt_helper_test.c)

# Include paths
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/zephyr/net/)

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
        -DCONFIG_MQTT_LIB_TLS=1
        -DCONFIG_NET_SOCKETS_POSIX_NAMES=1
        -DCONFIG_MQTT_HELPER_HOSTNAME="test-some-hostname.com"
        -DCONFIG_MQTT_HELPER_STATIC_IP_ADDRESS=""
        -DCONFIG_MQTT_HELPER_PORT=8883
        -DCONFIG_MQTT_HELPER_TIMEOUT_SEC=60
        -DCONFIG_MQTT_HELPER_RX_TX_BUFFER_SIZE=256
        -DCONFIG_MQTT_HELPER_PAYLOAD_BUFFER_LEN=2304
        -DCONFIG_MQTT_HELPER_STACK_SIZE=2560
        -DCONFIG_MQTT_HELPER_SEC_TAG=1
        -DCONFIG_MQTT_HELPER_SECONDARY_SEC_TAG=-1
        -DCONFIG_MQTT_HELPER_SEND_TIMEOUT_SEC=60
        -DCONFIG_MQTT_HELPER_LAST_WILL=y
        -DCONFIG_MQTT_HELPER_LAST_WILL_MESSAGE="lastwillmessage"
        -DCONFIG_MQTT_HELPER_LAST_WILL_TOPIC="lastwilltopic"
)
