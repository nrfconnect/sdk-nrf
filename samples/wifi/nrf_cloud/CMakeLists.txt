#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud)

# Exactly one of mqtt.conf or coap.conf must be selected
if(NOT CONFIG_NRF_CLOUD_MQTT AND NOT CONFIG_NRF_CLOUD_COAP)
  message(FATAL_ERROR
    "No nRF Cloud transport selected. Build with "
    "-DEXTRA_CONF_FILE=\"mqtt.conf\" or -DEXTRA_CONF_FILE=\"coap.conf\" (or a file list "
    "including one of them).")
elseif(CONFIG_NRF_CLOUD_MQTT AND CONFIG_NRF_CLOUD_COAP)
  message(FATAL_ERROR
    "Both nRF Cloud transports are enabled. Build with only one of "
    "-DEXTRA_CONF_FILE=\"mqtt.conf\" or -DEXTRA_CONF_FILE=\"coap.conf\".")
endif()

# Include certs files if enabled.
zephyr_include_directories_ifdef(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES certs)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/application.c)
target_sources(app PRIVATE src/cloud_connection.c)
target_sources(app PRIVATE src/message_queue.c)
target_sources(app PRIVATE src/temperature.c)
target_sources(app PRIVATE src/led_control.c)
target_sources(app PRIVATE src/sample_reboot.c)
target_sources(app PRIVATE src/shadow_config.c)

if(CONFIG_LOCATION_TRACKING)
  target_sources(app PRIVATE src/location_tracking.c)
endif()

if(CONFIG_COAP_SHADOW)
  target_sources(app PRIVATE src/shadow_support_coap.c)
endif()

if(CONFIG_WIFI_NRF_CLOUD_MEMFAULT_FOTA)
  target_sources(app PRIVATE src/memfault_fota_support.c)
endif()

# NORDIC SDK APP END

zephyr_include_directories(src)
