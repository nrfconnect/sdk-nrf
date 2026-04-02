#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_power_profiling)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/pwr_service.c
)

if(CONFIG_BT_EXT_ADV)
  target_sources(app PRIVATE src/advertising_ext.c)
else()
  target_sources(app PRIVATE src/advertising_legacy.c)
endif()
# NORDIC SDK APP END
