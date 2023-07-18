#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

if(DEFINED SHIELD AND "${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpunet")
  string(REPLACE " " ";" SHIELD_LIST "${SHIELD}")
  foreach(sh IN LISTS SHIELD_LIST)
    if("${sh}" STREQUAL "nrf21540ek")
      if(CONFIG_DTM_USB)
        set(remote_shell_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/nrf21540ek_usb.overlay)
      else()
        set(remote_shell_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/nrf21540ek.overlay)
      endif()

      break()
    endif()
  endforeach()
endif()

if(NOT DEFINED remote_shell_DTC_OVERLAY_FILE AND CONFIG_DTM_USB)
  set(remote_shell_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/usb.overlay)
endif()

if(CONFIG_DTM_USB)
  set(remote_shell_CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell/prj_usb.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/dtm.c
  src/dtm_hw.c
)
# NORDIC SDK APP END
