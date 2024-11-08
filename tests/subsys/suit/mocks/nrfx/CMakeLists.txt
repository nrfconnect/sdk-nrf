#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(suit_nrfx_mocks INTERFACE)

target_include_directories(suit_nrfx_mocks INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

if(TARGET zephyr)
  target_link_libraries(zephyr_interface INTERFACE suit_nrfx_mocks)
  zephyr_library_link_libraries(suit_nrfx_mocks)
elseif(TARGET testbinary)
  target_link_libraries(testbinary PRIVATE suit_nrfx_mocks)
else()
  message(FATAL_ERROR "Unknown test binary target name")
endif()
