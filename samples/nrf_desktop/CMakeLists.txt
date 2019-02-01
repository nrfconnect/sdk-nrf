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
  nrf52_pca63519
  )

set(NRF_SUPPORTED_BUILD_TYPES
  ZDebug
  ZDebugWithShell
  ZRelease
  ZReleaseMCUBoot
  )

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ZDebug)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}.overlay")
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}.overlay")
endif()

include(../../cmake/boilerplate.cmake)

# Define configuration files.
macro(set_conf_file)
  set(CONF_FILE "configuration/${BOARD}_${CMAKE_BUILD_TYPE}.conf")
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

if (${CONFIG_BOOTLOADER_MCUBOOT})
  if (NOT EXISTS ${MCUBOOT_DIR})
    message(FATAL_ERROR "Path to MCUBoot must be specified")
  endif()

  if (${CONFIG_DESKTOP_SIGNATURE_KEY_FILE})
    set(key_file_path ${CONFIG_DESKTOP_SIGNATURE_KEY_FILE_PATH})
  else()
    message(WARNING "Using default key file from MCUBoot")
    set(key_file_path "${MCUBOOT_DIR}/root-rsa-2048.pem")
  endif()

  set(cmd ${CMAKE_COMMAND} -E env
      ${MCUBOOT_DIR}/scripts/imgtool.py sign
      --key ${key_file_path}
      --header-size ${CONFIG_TEXT_SECTION_OFFSET}
      --align 8
      --version ${CMAKE_PROJECT_VERSION}
      --slot-size ${CONFIG_FLASH_LOAD_SIZE}
      ${APPLICATION_BINARY_DIR}/zephyr/zephyr.hex
      ${APPLICATION_BINARY_DIR}/zephyr/zephyr_signed.hex
      DEPENDS ${APPLICATION_BINARY_DIR}/zephyr/zephyr.elf
  )
  add_custom_target(sign
    COMMAND
    ${cmd}
    COMMENT
    "Sign file ${APPLICATION_BINARY_DIR}/zephyr/zephyr_signed.hex"
    USES_TERMINAL
    )
endif()
