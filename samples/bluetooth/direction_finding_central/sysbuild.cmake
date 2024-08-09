#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT SB_CONFIG_SOC_NRF5340_CPUAPP)
  if(NOT "bt-ll-sw-split" IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
    list(APPEND ${DEFAULT_IMAGE}_SNIPPET ${SNIPPET})
    list(APPEND ${DEFAULT_IMAGE}_SNIPPET bt-ll-sw-split)
    set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} CACHE STRING "" FORCE)
  endif()
  if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE AND NOT "bt-ll-sw-split" IN_LIST s1_image_SNIPPET)
    list(APPEND s1_image_SNIPPET ${SNIPPET})
    list(APPEND s1_image_SNIPPET bt-ll-sw-split)
    set(s1_image_SNIPPET ${s1_image_SNIPPET} CACHE STRING "" FORCE)
  endif()
endif()

if(NOT "bt-ll-sw-split" IN_LIST ipc_radio_SNIPPET)
  list(APPEND ipc_radio_SNIPPET ${SNIPPET})
  list(APPEND ipc_radio_SNIPPET bt-ll-sw-split)
  set(ipc_radio_SNIPPET ${ipc_radio_SNIPPET} CACHE STRING "" FORCE)
endif()
