#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

set(NRF_SUPPORTED_BOARDS
  nrf52840_pca20041
  nrf52840_pca10056
  nrf52840_pca10059
  nrf52_pca63519)

if (${BOARD} STREQUAL nrf52840_pca10056)
        set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}.overlay")
endif()

include(../../cmake/boilerplate.cmake)

# Define configuration files.
macro(set_conf_file)
  set(CONF_FILE
	"configuration/app_${BOARD}.conf"
	"configuration/nrfconnect.conf"
	"configuration/zephyr.conf"
	)
endmacro()

################################################################################

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("nRF52 Desktop"
        VERSION 0.1)

################################################################################

assert(CONFIG_DESKTOP_PORT_STATE  "Port state file must be specified")
assert(CONFIG_DESKTOP_REPORT_DESC "HID report descriptor file must be specified")

if (${CONFIG_DESKTOP_BUTTONS_NONE})
else()
	assert(CONFIG_DESKTOP_KEYMAP     "Keymap file must be specified")
	target_sources(app PRIVATE ${CONFIG_DESKTOP_KEYMAP})
endif()

if (${CONFIG_DESKTOP_LED_ENABLE})
	assert(CONFIG_DESKTOP_LED_STATE  "Led state file must be specified")
	target_sources(app PRIVATE ${CONFIG_DESKTOP_LED_STATE})
endif()

target_sources(app PRIVATE
  src/main.c
  ${CONFIG_DESKTOP_PORT_STATE}
  ${CONFIG_DESKTOP_REPORT_DESC}
)

# Include application events and configuration headers
zephyr_library_include_directories(
  src/events
  configuration
  )

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/hw_interface)
add_subdirectory(src/modules)
add_subdirectory(src/services)
