#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(location_test)

# Generate runner for the test
test_runner_generate(src/location_test.c)

cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/modem_key_mgmt.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/rest_client.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_gnss.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_at.h
	     FUNC_EXCLUDE ".*nrf_modem_at_printf"
             FUNC_EXCLUDE ".*nrf_modem_at_scanf")

cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/nrf_cloud.h
	     FUNC_EXCLUDE ".*nrf_cloud_obj_shadow_update")
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/nrf_cloud_agnss.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/nrf_cloud_rest.h)

# Net and Wi-Fi management API mocking requires tricks as the headers have code structures
# that CMock is not able to parse properly.
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/net_if.h
	     FUNC_EXCLUDE ".*net_if_send_data"
	     FUNC_EXCLUDE ".*net_if_recv_data"
	     FUNC_EXCLUDE ".*net_if_queue_tx"
	     FUNC_EXCLUDE ".*net_if_register_timestamp_cb"
	     FUNC_EXCLUDE ".*net_if_call_timestamp_cb"
	     FUNC_EXCLUDE ".*net_if_add_tx_timestamp"
	     FUNC_EXCLUDE ".*net_if_ipv4_get_netmask"
	     FUNC_EXCLUDE ".*net_if_ipv4_set_netmask"
	     WORD_EXCLUDE ".*deprecated.*"
	     WORD_EXCLUDE ".*struct net_pkt.*")
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/net_mgmt.h
	     FUNC_EXCLUDE ".*net_mgmt_event_notify"
	     FUNC_EXCLUDE ".*net_mgmt_event_notify_with_info"
	     FUNC_EXCLUDE ".*net_mgmt_event_wait_on_iface"
	     WORD_EXCLUDE ".*struct net_if.*")
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/wifi_mgmt.h
	     FUNC_EXCLUDE "NET_MGMT_DEFINE_REQUEST_HANDLER"
	     FUNC_EXCLUDE "BUILD_ASSERT"
	     FUNC_EXCLUDE "offsetof")
cmock_handle(${ZEPHYR_BASE}/include/zephyr/device.h
	     FUNC_EXCLUDE ".*DEVICE_DT_NAME_GET"
	     FUNC_EXCLUDE ".*device_get_dt_nodelabels"
	     WORD_EXCLUDE ".*device_visitor_callback_t.*")

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/include)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/net)

# The test uses double precision floating point numbers. This is not enabled by default in unity
# unless we set the following define.
zephyr_compile_definitions(UNITY_INCLUDE_DOUBLE)

target_sources(app PRIVATE src/location_test.c)

target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/net)

# This is needed due to parsing issues in CMock for static inline declarations
# and caused by declaration of net_if_flag_is_set():
# [41/147] Building C object CMakeFiles/app.dir/mocks/cmock_net_if.c.obj
# In file included from nrf/tests/lib/location/build/mocks/cmock_net_if.h:6,
#                  from nrf/tests/lib/location/build/mocks/cmock_net_if.c:5:
# nrf/tests/lib/location/build/mocks/net_if.h:540:20: warning: ‘net_if_tx_lock’
#         declared ‘static’ but never defined [-Wunused-function]
#   540 | static inline void net_if_tx_lock(struct net_if *iface);
target_compile_options(app PRIVATE "-Wno-unused-function")

# Extend autoconf.h for nrf/lib/location
# Allows KConfig options to be enabled for nrf/lib/location without affecting CMake
# ..__nrf__lib__location is the name assigned to nrf/lib/location by the zephyr_library() macro in
# zephyr/cmake/modules/extensions.cmake

if (CONFIG_LOCATION_TEST_AGNSS)

target_compile_options(
	..__nrf__lib__location PRIVATE
	"SHELL: -imacros ${PROJECT_SOURCE_DIR}/location_lib_autoconf_ext.h"
	"SHELL: -imacros ${PROJECT_SOURCE_DIR}/location_lib_autoconf_ext_agnss.h"
)
else()

target_compile_options(
	..__nrf__lib__location PRIVATE
	"SHELL: -imacros ${PROJECT_SOURCE_DIR}/location_lib_autoconf_ext.h"
)
endif()

# Add definitions also for test application.
# A-GNSS definitions do not distract when using tests without A-GNSS.
target_compile_options(
	app PRIVATE
	"SHELL: -imacros ${PROJECT_SOURCE_DIR}/location_lib_autoconf_ext.h"
	"SHELL: -imacros ${PROJECT_SOURCE_DIR}/location_lib_autoconf_ext_agnss.h"
)
