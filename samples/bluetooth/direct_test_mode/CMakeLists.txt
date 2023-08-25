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

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpunet")
  if(NOT "${EXTRA_CONF_FILE}" STREQUAL "overlay-hci-nrf53.conf")
    set(EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/conf/remote_shell_nrf53.overlay")
  endif()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

target_include_directories(app PRIVATE ./src)

target_sources_ifdef(CONFIG_DTM_TRANSPORT_TWOWIRE app PRIVATE
  src/transport/dtm_uart_twowire.c
  src/transport/dtm_uart_wait.c
)

target_sources_ifdef(CONFIG_DTM_TRANSPORT_HCI app PRIVATE src/transport/dtm_hci.c)

if(CONFIG_NCS_SAMPLE_DTM_REMOTE_HCI_CHILD_IMAGE)
  target_sources(app PRIVATE src/transport/hci_uart_remote.c)
  target_include_directories(app PRIVATE ./rpc)
else()
  target_sources_ifdef(CONFIG_DTM_TRANSPORT_HCI app PRIVATE src/transport/hci_uart.c)
endif()

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/dtm.c
  src/dtm_hw.c
  src/main.c
)
# NORDIC SDK APP END
