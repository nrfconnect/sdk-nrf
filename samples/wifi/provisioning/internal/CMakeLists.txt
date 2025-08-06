#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_prov_internal)

# WiFi Provisioning Configuration
if(CONFIG_WIFI_PROV_CONFIG)
    # Generate sample WiFi configuration
    set(PROTO_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/wifi_prov_core/proto)
    set(WIFI_CONFIG_SCRIPT ${PROTO_DIR}/generate_wifi_prov_config.py)
    set(WIFI_CONFIG_BIN ${CMAKE_CURRENT_BINARY_DIR}/wifi_config.bin)
    set(WIFI_CONFIG_INC ${CMAKE_CURRENT_BINARY_DIR}/wifi_config.inc)

    # Note: nanopb generation is already handled by wifi_prov_core library
    # which is linked to the app target below

    # Build the command with conditional certificate directory
    set(WIFI_CONFIG_CMD ${Python3_EXECUTABLE} ${WIFI_CONFIG_SCRIPT}
        "${CONFIG_WIFI_PROV_SSID}" "${CONFIG_WIFI_PROV_BSSID}"
        -w "${CONFIG_WIFI_PROV_PASSPHRASE}"
        -a ${CONFIG_WIFI_PROV_AUTH_MODE} -c ${CONFIG_WIFI_PROV_CHANNEL} -b ${CONFIG_WIFI_PROV_BAND}
        -o ${WIFI_CONFIG_BIN})

    # Add certificate directory if specified
    if(CONFIG_WIFI_PROV_CERT_DIR)
        list(APPEND WIFI_CONFIG_CMD -d "${CONFIG_WIFI_PROV_CERT_DIR}")
    endif()

    # Generate the binary file
    add_custom_command(
        OUTPUT ${WIFI_CONFIG_BIN}
        COMMAND ${WIFI_CONFIG_CMD}
        DEPENDS ${WIFI_CONFIG_SCRIPT}
        DEPENDS wifi_prov_core
        WORKING_DIRECTORY ${PROTO_DIR}
        COMMENT "Generating sample WiFi configuration"
    )

    # Use Zephyr's generate_inc_file_for_target to create the .inc file
    generate_inc_file_for_target(app ${WIFI_CONFIG_BIN} ${WIFI_CONFIG_INC})

    # Include generated header directory
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    # Include WiFi provisioning library headers and link against it
    target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/wifi_prov_core)
    target_include_directories(app PRIVATE $<TARGET_PROPERTY:wifi_prov_core,BINARY_DIR>)
    target_include_directories(app PRIVATE ${ZEPHYR_BASE}/modules/lib/nanopb)
    target_link_libraries(app PRIVATE wifi_prov_core)
endif() # CONFIG_WIFI_PROV_CONFIG

target_sources(app PRIVATE
    src/main.c
    src/prov.c
    src/wifi_prov_transport_stub.c
)
