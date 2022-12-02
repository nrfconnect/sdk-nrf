#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

set(PM_STATIC_YML_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/boards/pm_static_${BOARD}.yml
  )

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)

# Preinitialization related to Thingy:53 DFU
target_sources_ifdef(CONFIG_BOARD_THINGY53_NRF5340_CPUAPP app PRIVATE
  boards/thingy53.c
)
target_sources_ifdef(CONFIG_BOARD_THINGY53_NRF5340_CPUAPP_NS app PRIVATE
  boards/thingy53.c
)


# NORDIC SDK APP END
zephyr_library_include_directories(.)
