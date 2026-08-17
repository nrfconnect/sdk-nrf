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

# Give the fatal error report a channel of its own on the network core side. The HCI endpoint
# cannot carry it, because the RPMsg backend serving that endpoint takes a mutex and hands the
# message to a thread, while the report is made with interrupts locked and possibly from an
# interrupt context. Only the nRF5340 memory layout is described here, so other targets fall back
# to reporting over the HCI endpoint.
if(SB_CONFIG_SOC_NRF5340_CPUAPP)
  set(netcore_image ${SB_CONFIG_NETCORE_IMAGE_NAME})

  set(${netcore_image}_EXTRA_DTC_OVERLAY_FILE
      "${${netcore_image}_EXTRA_DTC_OVERLAY_FILE};${CMAKE_CURRENT_LIST_DIR}/dts/nrf5340_ipc_fatal.overlay"
      CACHE STRING "Extra devicetree overlays for the ${netcore_image} image" FORCE
  )
endif()
