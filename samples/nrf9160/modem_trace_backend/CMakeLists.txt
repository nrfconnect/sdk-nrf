#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_trace_backend)

if(CONFIG_NRF_MODEM_LIB_TRACE)

# Only add 'custom' backend to compilation when selected.
zephyr_library_sources_ifdef(
  CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_CUSTOM
  src/trace_print_stats.c
)

endif()

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

# NORDIC SDK APP END
