#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(HW_VERSION_MAJOR "3")
set(HW_VERSION_MINOR "0")
set(FW_VERSION_MAJOR "1")
set(FW_VERSION_MINOR "6.0")

set(spm_CONF_FILE
    prj.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}_spm.conf)

# enable ability to perform FOTA from NCS 1.2.0 gw to newer
# e.g., west build -p auto -b apricity_gateway_nrf9160ns -d build_dbg -- -DFOTA_OLD_GW=y
if(FOTA_OLD_GW)
				set(HW_VERSION_MAJOR "2")
				set(HW_VERSION_MINOR "0")
				set(PM_STATIC_YML_FILE
					${CMAKE_CURRENT_SOURCE_DIR}/old_gw_pm_static.yml
					)
				message("FOTA for old gw")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_ble_gateway)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# Include certs files if enabled.
zephyr_include_directories_ifdef(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES certs)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/application.c)
target_sources(app PRIVATE src/cloud_connection.c)
target_sources(app PRIVATE src/message_queue.c)
target_sources(app PRIVATE src/temperature.c)
target_sources(app PRIVATE src/fota_support.c)
target_sources(app PRIVATE src/led_control.c)
target_sources(app PRIVATE src/sample_reboot.c)
target_sources(app PRIVATE src/shadow_config.c)

if(CONFIG_LOCATION_TRACKING)
target_sources(app PRIVATE src/location_tracking.c)
endif()

if(CONFIG_COAP_SHADOW)
target_sources(app PRIVATE src/shadow_support_coap.c)
endif()

if(CONFIG_COAP_FOTA)
target_sources(app PRIVATE src/fota_support_coap.c)
endif()

if(CONFIG_NRF_PROVISIONING)
target_sources(app PRIVATE src/provisioning_support.c)
endif()

# Include modem-specific features if supported
if(CONFIG_NRF_MODEM_LIB)
	target_sources(app PRIVATE src/at_commands.c)
endif()

find_package(Git QUIET REQUIRED)

execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --always HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE res
    OUTPUT_VARIABLE MYAPP_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)

set_property(GLOBAL APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/.git/index")

if ("${MYAPP_VERSION}" MATCHES "^.*-(.*)-g.*$")
    # if HEAD is tagged with a form of major.minor.patch this separates the items and uses them instead
    string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" "\\1;\\2;\\3" _ver_parts "${MYAPP_VERSION}")
#    list(GET _ver_parts 0 FW_VERSION_MAJOR)
#    list(GET _ver_parts 1 FW_VERSION_MINOR)
#    list(GET _ver_parts 2 FW_VERSION_PATCH)
else ()
    set(FW_VERSION_PATCH ${MYAPP_VERSION})
endif ()

#
# utilize settings to autogenerate a header file to adjust the build inside the C code
#
configure_file (
   "src/ble/config.h.in"
   "${PROJECT_SOURCE_DIR}/src/ble/config.h"
   )

target_sources(app PRIVATE src/ble/ble.c)
target_sources(app PRIVATE src/ble/ble_codec.c)
target_sources(app PRIVATE src/ble/ble_conn_mgr.c)
target_sources(app PRIVATE src/ble/gateway.c)
target_sources_ifdef(CONFIG_SHELL app PRIVATE src/ble/cli.c)
target_sources_ifdef(CONFIG_GATEWAY_BLE_FOTA app PRIVATE src/ble/dfu/peripheral_dfu.c)

add_subdirectory(src/ble/ui)
#add_subdirectory(src/ble/dfu)
add_subdirectory(src/ble/flash)

zephyr_include_directories(src src/ble src/ble/ui src/ble/flash src/ble/dfu)

# NORDIC SDK APP END
