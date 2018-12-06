#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_HAS_HW_NRF_KMU)
  message(FATAL_ERROR "No KMU support added.")
else()
  set(PROVISION_SRC provision_flash.c)
endif()

add_library(
  provision
  ${PROVISION_SRC})

# SB_TODO Can we get the errno.h from somewhere else?
target_include_directories(provision PRIVATE BEFORE ${ZEPHYR_BASE}/lib/libc/minimal/include)
target_link_libraries(provision PRIVATE zephyr_interface)
# Make sure every generated file is present before trying to build the
# library.
target_link_libraries(provision PRIVATE bootloader)
