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
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/net_if.h zephyr/net
  FUNC_EXCLUDE ".*net_if_send_data"
  FUNC_EXCLUDE ".*net_if_try_send_data"
  FUNC_EXCLUDE ".*net_if_recv_data"
  FUNC_EXCLUDE ".*net_if_queue_tx"
  FUNC_EXCLUDE ".*net_if_try_queue_tx"
  FUNC_EXCLUDE ".*net_if_register_timestamp_cb"
  FUNC_EXCLUDE ".*net_if_unregister_timestamp_cb"
  FUNC_EXCLUDE ".*net_if_call_timestamp_cb"
  FUNC_EXCLUDE ".*net_if_add_tx_timestamp"
  FUNC_EXCLUDE ".*net_if_set_link_addr_locked"
  FUNC_EXCLUDE ".*net_if_is_wifi"
  FUNC_EXCLUDE ".*net_if_set_name"
  FUNC_EXCLUDE ".*net_if_ipv6_calc_reachable_time"
  FUNC_EXCLUDE ".*net_if_ipv4_get_netmask"
  FUNC_EXCLUDE ".*net_if_ipv4_set_netmask"
  FUNC_EXCLUDE ".*net_if_offload_set"
  WORD_EXCLUDE ".*deprecated.*"
  WORD_EXCLUDE ".*net_if_send_data.*"
  WORD_EXCLUDE ".*net_if_try_send_data.*"
  WORD_EXCLUDE ".*net_if_try_queue_tx.*"
  WORD_EXCLUDE ".*net_if_register_timestamp_cb.*"
  WORD_EXCLUDE ".*net_if_set_link_addr_locked.*"
  WORD_EXCLUDE ".*net_if_is_wifi.*"
  WORD_EXCLUDE ".*net_if_set_name.*"
  WORD_EXCLUDE ".*net_if_ipv6_calc_reachable_time.*"
)

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
  -DCONFIG_POSIX_API=1
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
