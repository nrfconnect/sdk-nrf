#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aws_iot_test)

# Generate runner for the test
test_runner_generate(src/aws_iot_test.c)

# Create mock
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/mqtt_helper.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/aws_fota.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/dfu/dfu_target.h)
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/mqtt.h)

# Add Unit Under Test source files
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/aws_iot/src/aws_iot.c)

# Add test source file
target_sources(app PRIVATE src/aws_iot_test.c)

# Include paths
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/aws_iot/include/)
target_include_directories(app PRIVATE src)

# Preinclude file to the UUT to redefine IS_ENABLED().
set_property(SOURCE ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/aws_iot/src/aws_iot.c
	     PROPERTY COMPILE_FLAGS "-include redefinitions.h")

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
	-DCONFIG_AWS_IOT_BROKER_HOST_NAME="test.amazonaws.com"
	-DCONFIG_AWS_IOT_CLIENT_ID_STATIC="test-client-id"
	-DCONFIG_AWS_IOT_CLIENT_ID_MAX_LEN=40
	-DCONFIG_AWS_IOT_CONNECT_TIMEOUT_SECONDS=30
	-DCONFIG_AWS_IOT_AUTO_DEVICE_SHADOW_REQUEST=y
	-DCONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT=9
	-DCONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE=y
	-DCONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE=y
	-DCONFIG_AWS_IOT_TOPIC_UPDATE_ACCEPTED_SUBSCRIBE=y
	-DCONFIG_AWS_IOT_TOPIC_UPDATE_REJECTED_SUBSCRIBE=y
	-DCONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE=y
	-DCONFIG_AWS_IOT_TOPIC_DELETE_ACCEPTED_SUBSCRIBE=y
	-DCONFIG_AWS_IOT_TOPIC_DELETE_REJECTED_SUBSCRIBE=y
	-DCONFIG_AWS_IOT_LAST_WILL=y
	-DCONFIG_AWS_IOT_LAST_WILL_MESSAGE="{\"lastwill\": \"offline\"}"
	-DCONFIG_AWS_IOT_LAST_WILL_TOPIC="lastwilltopic"
	-DCONFIG_MQTT_HELPER_PORT=8883
	-DCONFIG_MQTT_HELPER_SEND_TIMEOUT_SEC=60
	-DCONFIG_MQTT_HELPER_STATIC_IP_ADDRESS=""
	-DCONFIG_MQTT_HELPER_RX_TX_BUFFER_SIZE=256
	-DCONFIG_MQTT_HELPER_PAYLOAD_BUFFER_LEN=2304
	-DCONFIG_MQTT_HELPER_SEC_TAG=1
	-DCONFIG_MQTT_HELPER_STACK_SIZE=2560
)
