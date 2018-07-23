#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

# Check if selected board is supported.
if    (${BOARD} STREQUAL "nrf52840_pca20041")
elseif(${BOARD} STREQUAL "nrf52840_pca10056")
elseif(${BOARD} STREQUAL "nrf52_pca63519")
else()
	message(FATAL_ERROR "board ${BOARD} is not supported")
endif()

# Point to NCS root directory.
set(NRF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Application is root for the entire configuration and binds
# its own, kernel and NCS configuration trees.
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

# Define configuration files.
set(CONF_FILE "configuration/app_${BOARD}.conf")
list(APPEND CONF_FILE "configuration/nrfconnect.conf")
list(APPEND CONF_FILE "configuration/zephyr.conf")

# Use boards defined in NCS.
if ((${BOARD} STREQUAL "nrf52840_pca20041") OR
    (${BOARD} STREQUAL "nrf52_pca63519"))
	set(BOARD_ROOT ${NRF_DIR})
endif()

################################################################################

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("nRF52 Desktop"
        VERSION 0.1)

################################################################################

# Use key map specified in configuration.
if(${CONFIG_DESKTOP_KEYMAP} STREQUAL "")
	message(FATAL_ERROR "Keymap file must be specified")
else()
	target_sources(app PRIVATE
                       ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DESKTOP_KEYMAP})
endif()

# Include application events
zephyr_library_include_directories(src/events)
# Include configuration headers
zephyr_library_include_directories(configuration)

# NCS sources
#add_subdirectory(${NRF_DIR} ${PROJECT_BINARY_DIR}/nrf)

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/hw_interface)
add_subdirectory(src/modules)
add_subdirectory(src/services)

target_sources(app PRIVATE src/main.c)
