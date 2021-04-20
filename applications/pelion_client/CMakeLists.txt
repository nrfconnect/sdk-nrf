#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)


if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ZDebug)
endif()

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}")
  message(FATAL_ERROR
          "Board ${BOARD} is not supported.\n"
          "Please make sure board specific configuration files are added to "
          "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}")
endif()

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")
  message(FATAL_ERROR
          "Configuration file for build type ${CMAKE_BUILD_TYPE} is missing.\n"
          "Please add file ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
endif()

set(mcuboot_CONF_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/mcuboot_${CMAKE_BUILD_TYPE}.conf
  )

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/spm_${CMAKE_BUILD_TYPE}.conf)
  set(spm_CONF_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/spm_${CMAKE_BUILD_TYPE}.conf
    )
endif()

# Use static partition layout to ensure consistency between builds.
# This is to ensure settings storage will be at the same location after the DFU.
set(PM_STATIC_YML_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_${CMAKE_BUILD_TYPE}.yml
  )

# Define configuration files.
set(CONF_FILE "configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")

################################################################################

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("PelionClient" VERSION 0.1)

################################################################################

target_sources(app PRIVATE
  src/main.c
  )

target_sources(app PRIVATE
  configuration/common/mbed_cloud_dev_credentials.c
  )

target_sources_ifdef(CONFIG_PELION_UPDATE_DEVELOPER app PRIVATE
  configuration/common/update_default_resources.c
  )

# Include application events and configuration headers
zephyr_library_include_directories(
  src/util
  src/events
  )

zephyr_include_directories(
  configuration/common
  configuration/${BOARD}
  )

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/modules)
add_subdirectory(src/util)

if(CONFIG_BOOTLOADER_MCUBOOT)
  assert_exists(mcuboot_CONF_FILE)
endif()

if(CONFIG_PELION_MBEDTLS)
  # Copy configuration file to include directory visible for all libraries.
  # Needed for the file to be visible by Zephyr's mbedtls library.
  configure_file("configuration/${BOARD}/config-tls-pelion.h" "${CMAKE_BINARY_DIR}/zephyr/include/generated" COPYONLY)
endif()
