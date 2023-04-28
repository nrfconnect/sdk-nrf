#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if (DEFINED SHIELD AND ${BOARD} STREQUAL "nrf5340dk_nrf5340_cpunet")
  string(REPLACE " " ";" SHIELD_LIST "${SHIELD}")
  foreach(sh IN_LIST SHIELD_LIST)
    if (${sh} STREQUAL "nrf21540_ek")
      if (CONFIG_RADIO_TEST_USB)
        set(remote_shell_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/nrf21540_ek_usb.overlay)
      else()
        set(remote_shell_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/nrf21540_ek.overlay)
      endif()

      break()
    endif()
  endforeach()
endif()

if (NOT DEFINED remote_shell_DTC_OVERLAY_FILE AND CONFIG_RADIO_TEST_USB)
  set(remote_shell_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/usb.overlay)
endif()

if (CONFIG_RADIO_TEST_USB)
  set(remote_shell_CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/prj_usb.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# Application sources
FILE(GLOB app_sources src/*.c)

# NORDIC SDK APP START
target_sources(app PRIVATE
	${app_sources}
)
# NORDIC SDK APP END
