#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(suit_sdfw_mocks INTERFACE)

target_include_directories(suit_sdfw_mocks INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

if(CONFIG_MOCK_SDFW_ARBITER)
  target_compile_options(suit_sdfw_mocks INTERFACE -DCONFIG_SDFW_ARBITER)
endif()
if(CONFIG_MOCK_SDFW_RESET_MGR)
  target_compile_options(suit_sdfw_mocks INTERFACE -DCONFIG_SDFW_RESET_HANDLING_ENABLED)
endif()
if(CONFIG_MOCK_SDFW_VPRS)
  target_compile_options(suit_sdfw_mocks INTERFACE -DCONFIG_SDFW_VPRS)
endif()

if(TARGET zephyr)
  target_link_libraries(zephyr_interface INTERFACE suit_sdfw_mocks)
  zephyr_library_link_libraries(suit_sdfw_mocks)
elseif(TARGET testbinary)
  target_link_libraries(testbinary PRIVATE suit_sdfw_mocks)
else()
  message(FATAL_ERROR "Unknown test binary target name")
endif()
