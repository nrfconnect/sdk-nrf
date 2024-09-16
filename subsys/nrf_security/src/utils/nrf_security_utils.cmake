#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(nrf_security_utils STATIC
  ${CMAKE_CURRENT_LIST_DIR}/nrf_security_mutexes.c
  ${CMAKE_CURRENT_LIST_DIR}/nrf_security_events.c
)

target_include_directories(nrf_security_utils
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# This special linking is done to give access to the zephyr kernel library
# which possibly isn't --whole-archived in the build. Trying to link to the 
# kernel library directly will give cyclic dependency. The only way to avoid
# it seems to be to link with a full path instead.
target_link_libraries(nrf_security_utils
  PRIVATE
    ${CMAKE_BINARY_DIR}/zephyr/kernel/libkernel.a
)

nrf_security_add_zephyr_options_library(nrf_security_utils)
