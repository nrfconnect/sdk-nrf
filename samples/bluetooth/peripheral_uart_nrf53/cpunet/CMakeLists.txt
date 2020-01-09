#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
cmake_minimum_required(VERSION 3.8)


set(NRF_SUPPORTED_BOARDS
  nrf5340_dk_nrf5340_cpunet
)

include(../../../../cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  ../common/bt_ser.c
  ../common/rpmsg.c
)

# NORDIC SDK APP END

target_link_libraries(app PRIVATE TINYCBOR)
zephyr_library_include_directories(.)
zephyr_library_include_directories(../common)
