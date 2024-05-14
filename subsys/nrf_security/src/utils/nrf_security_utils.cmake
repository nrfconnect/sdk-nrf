#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

list(APPEND nrf_security_utils_sources
  ${CMAKE_CURRENT_LIST_DIR}/nrf_security_mutexes.c
  ${CMAKE_CURRENT_LIST_DIR}/nrf_security_events.c
)

list(APPEND nrf_security_utils_include_dirs
    ${CMAKE_CURRENT_LIST_DIR}
)
