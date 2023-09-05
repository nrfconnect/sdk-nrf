#
# Copyright (c) 2018 - 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

################################################################################

# The application uses the configuration/<board> scheme for configuration files.
set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/configuration/\${BOARD}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("nRF52 Desktop"
        VERSION 0.1)

################################################################################

assert(CONFIG_DESKTOP_HID_REPORT_DESC "HID report descriptor file must be specified")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  ${CONFIG_DESKTOP_HID_REPORT_DESC}
  )
# NORDIC SDK APP END

# Include application events and configuration headers
zephyr_library_include_directories(
  src/events
  src/util
  )

zephyr_include_directories(
  configuration/common
  ${APPLICATION_CONFIG_DIR}
  )

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/hw_interface)
add_subdirectory(src/modules)
add_subdirectory(src/util)

if(NOT SYSBUILD AND NOT CONFIG_NCS_IS_VARIANT_IMAGE)
  if(CONFIG_BOOTLOADER_MCUBOOT)
    assert_exists(mcuboot_CONF_FILE)
  endif()
  if(CONFIG_SECURE_BOOT)
    assert_exists(b0_CONF_FILE)
  endif()
  if(CONFIG_BT_RPMSG)
    assert_exists(hci_rpmsg_CONF_FILE)
  endif()
endif()

if (CONFIG_IMG_MANAGER)
  zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()

if(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)
  zephyr_linker_sources(SECTIONS nrf_desktop.ld)
endif()
