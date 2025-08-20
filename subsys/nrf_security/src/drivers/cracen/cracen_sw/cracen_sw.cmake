#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This directory contains C sources for the CRACEN software workarounds

list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/include
)

if(CONFIG_CRACEN_NEED_MULTIPART_WORKAROUNDS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_sw_common.c
  )
endif()

if(CONFIG_CRACEN_NEED_MULTIPART_WORKAROUNDS AND CONFIG_PSA_NEED_CRACEN_MAC_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_sw_mac.c
  )

  if(CONFIG_PSA_NEED_CRACEN_CMAC)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/cracen_sw_mac_cmac.c
    )
  endif()
endif()
