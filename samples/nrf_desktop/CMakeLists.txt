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
  )

set(NRF_SUPPORTED_BUILD_TYPES
  ZDebug
  ZDebugWithShell
  ZDebugMCUBoot
  ZRelease
  ZReleaseMCUBoot
  )

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ZDebug)
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
endif()

# Adding proper flash map if overlay exists.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/flash.overlay")
  if(DTC_OVERLAY_FILE)
    set(DTC_OVERLAY_FILE "${DTC_OVERLAY_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/flash.overlay")
  else()
    set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/flash.overlay")
  endif()
endif()

include(../../cmake/boilerplate.cmake)

# Define configuration files.
macro(set_conf_file)
  set(CONF_FILE "configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")
endmacro()

################################################################################

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("nRF52 Desktop"
        VERSION 0.1)

################################################################################

assert(CONFIG_DESKTOP_PORT_STATE  "Port state file must be specified")
assert(CONFIG_DESKTOP_REPORT_DESC "HID report descriptor file must be specified")

if (${CONFIG_DESKTOP_HID_STATE_ENABLE})
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
  configuration/common
  configuration/${BOARD}
  )

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/hw_interface)
add_subdirectory(src/modules)
add_subdirectory(src/services)

if (${CONFIG_BOOTLOADER_MCUBOOT})
  set(MCUBOOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../mcuboot)

  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/mcuboot_${CMAKE_BUILD_TYPE}.conf")
    message(FATAL_ERROR "Configuration ${BOARD}/mcuboot_${CMAKE_BUILD_TYPE}.conf must be specified")
  endif()

  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
    message(FATAL_ERROR "Overlay ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay must be specified")
  endif()

  if (${CONFIG_DESKTOP_SIGNATURE_KEY_FILE})
    set(key_file_path ${CONFIG_DESKTOP_SIGNATURE_KEY_FILE_PATH})
  else()
    message(WARNING "Using default key file from MCUBoot")
    set(key_file_path "${MCUBOOT_DIR}/root-rsa-2048.pem")
  endif()

  set(cmd_sign_hex ${CMAKE_COMMAND} -E env
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

  set(cmd_sign_bin ${CMAKE_COMMAND} -E env
      ${MCUBOOT_DIR}/scripts/imgtool.py sign
      --key ${key_file_path}
      --header-size ${CONFIG_TEXT_SECTION_OFFSET}
      --align 8
      --version ${CMAKE_PROJECT_VERSION}
      --slot-size ${CONFIG_FLASH_LOAD_SIZE}
      ${APPLICATION_BINARY_DIR}/zephyr/zephyr.bin
      ${APPLICATION_BINARY_DIR}/zephyr/zephyr_signed.bin
      DEPENDS ${APPLICATION_BINARY_DIR}/zephyr/zephyr.elf
  )

  add_custom_target(sign
    COMMAND
    ${cmd_sign_hex}
    COMMAND
    ${cmd_sign_bin}
    COMMENT
    "Sign binary files"
    USES_TERMINAL
    )
  set(BOARD_ROOT_STR "")
  foreach(root ${BOARD_ROOT})
    set(BOARD_ROOT_STR "${BOARD_ROOT_STR}\;${root}")
  endforeach()

  add_custom_target(mcuboot
    COMMAND
    rm -rf mcuboot && mkdir mcuboot && cd mcuboot
    COMMAND
    cmake ${MCUBOOT_DIR}/boot/zephyr/
    -G${CMAKE_GENERATOR}
    -DBOARD=${BOARD}
    -DBOARD_ROOT="${BOARD_ROOT_STR}"
    -DDTC_OVERLAY_FILE="${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/flash.overlay"
    -DCONF_FILE="${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/mcuboot_${CMAKE_BUILD_TYPE}.conf"
    COMMENT
    "Generating bootloader for application in mcuboot folder"
    USES_TERMINAL
    )

endif()
