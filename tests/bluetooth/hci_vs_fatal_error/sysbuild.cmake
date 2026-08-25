#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_NETCORE_HCI_IPC)
  # Run the open source Zephyr Controller instead of the SoftDevice Controller that the network
  # core board target chooses by default.
  set(hci_ipc_SNIPPET bt-ll-sw-split CACHE INTERNAL "")
endif()
