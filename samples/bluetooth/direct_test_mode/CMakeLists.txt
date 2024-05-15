#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

# Zephyr doesn't support application specific shield overlays.
# Therefore pass shiled information to the board overlay as a define so that
# so that the shield overlay is included if required by the board overlay.
string(REPLACE " " ";" shield_list "${SHIELD}")
if("nrf21540ek" IN_LIST shield_list)
  set(remote_shell_DTS_EXTRA_CPPFLAGS "-DNRF21540EK")
endif()

if(DEFINED FILE_SUFFIX AND NOT DEFINED remote_shell_FILE_SUFFIX)
  set(remote_shell_FILE_SUFFIX ${FILE_SUFFIX})
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
