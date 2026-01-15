#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This directory contains C sources for the common functionality of the CRACEN crypto driver

list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/include
)

list(APPEND cracen_driver_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/security/cracen.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/mem_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/ec_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/prng_pool.c
)

if(CONFIG_CRACEN_LIB_KMU)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen/lib_kmu.c
  )
endif()
