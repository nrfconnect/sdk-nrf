#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc_radio)

target_include_directories(app PRIVATE ./src)

target_sources_ifdef(CONFIG_IPC_RADIO_802154 app PRIVATE src/802154.c)

if(CONFIG_IPC_RADIO_BT_HCI_IPC)
  target_sources(app PRIVATE src/bt_hci_ipc.c)
else()
  target_sources(app PRIVATE src/bt_empty.c)
endif()

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)
# NORDIC SDK APP END
