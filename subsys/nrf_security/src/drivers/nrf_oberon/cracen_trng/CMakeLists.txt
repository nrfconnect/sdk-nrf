#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (PSA_NEED_CRACEN_TRNG_DRIVER)
  target_sources(psa_core
    PRIVATE
      cracen_trng.c
  )
  target_include_directories(psa_core
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/..
  )
endif()
