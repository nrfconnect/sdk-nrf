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
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/hardware/hardware.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/common.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/cracen_rndinrange.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/mem_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/ec_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/prng_pool.c
)

if(CONFIG_PSA_NEED_CRACEN_HMAC OR CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen/cracen_hmac.c
  )
endif()

if(CONFIG_CRACEN_LIB_KMU)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen/lib_kmu.c
  )
endif()
