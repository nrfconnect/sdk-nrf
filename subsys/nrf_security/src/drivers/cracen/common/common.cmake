#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This directory contains C sources for the common functionality of the CRACEN crypto driver

list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/include
)

# KMU slot layout header. CRACEN_KMU_LAYOUT_FILE selects a header that replaces the
# default layout. It is read from sysbuild, so that all images share the same layout,
# and forwarded to the TF-M build. The selected header is copied into the build tree.
if(NOT BUILD_INSIDE_TFM)
  zephyr_get(CRACEN_KMU_LAYOUT_FILE SYSBUILD GLOBAL)
endif()

if(CRACEN_KMU_LAYOUT_FILE)
  if(NOT IS_ABSOLUTE ${CRACEN_KMU_LAYOUT_FILE})
    message(FATAL_ERROR "CRACEN_KMU_LAYOUT_FILE must be an absolute path: "
                        "${CRACEN_KMU_LAYOUT_FILE}")
  endif()
  if(NOT EXISTS ${CRACEN_KMU_LAYOUT_FILE})
    message(FATAL_ERROR "CRACEN_KMU_LAYOUT_FILE does not exist: ${CRACEN_KMU_LAYOUT_FILE}")
  endif()
  set(cracen_kmu_layout_file ${CRACEN_KMU_LAYOUT_FILE})
else()
  set(cracen_kmu_layout_file ${CMAKE_CURRENT_LIST_DIR}/include/cracen/cracen_kmu_layout_default.h)
endif()

configure_file(${cracen_kmu_layout_file}
  ${CMAKE_CURRENT_BINARY_DIR}/cracen_kmu_layout/cracen_kmu_layout_config.h
  COPYONLY
)

list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_BINARY_DIR}/cracen_kmu_layout
)

list(APPEND cracen_driver_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/hardware/hardware.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/common.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/cracen_rndinrange.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/ec_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen/prng_pool.c
)

if(CONFIG_PSA_NEED_CRACEN_HMAC OR CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen/cracen_hmac.c
  )
endif()
