#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This directory contains external C sources for the CRACEN

list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}
)

if(CONFIG_PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS AND CONFIG_PSA_NEED_CRACEN_GCM_AES)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/gcm_ext.c
  )
endif()
