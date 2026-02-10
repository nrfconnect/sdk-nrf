#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SOC_SERIES_NRF52)
  if(NOT "bt-ll-sw-split" IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
    list(APPEND ${DEFAULT_IMAGE}_SNIPPET ${SNIPPET})
    list(APPEND ${DEFAULT_IMAGE}_SNIPPET bt-ll-sw-split)
    set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} CACHE STRING "" FORCE)
  endif()
endif()

if(NOT "bt-ll-sw-split" IN_LIST ipc_radio_SNIPPET)
  list(APPEND ipc_radio_SNIPPET ${SNIPPET})
  list(APPEND ipc_radio_SNIPPET bt-ll-sw-split)
  set(ipc_radio_SNIPPET ${ipc_radio_SNIPPET} CACHE STRING "" FORCE)
endif()
