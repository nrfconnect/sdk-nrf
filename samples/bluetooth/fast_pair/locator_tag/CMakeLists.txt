#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED FP_MODEL_ID AND NOT DEFINED FP_ANTI_SPOOFING_KEY AND NOT SYSBUILD)
  message(WARNING "
  -------------------------------------------------------
  --- WARNING: Using demo Fast Pair Model ID and Fast ---
  --- Pair Anti Spoofing Key, it should not be used   ---
  --- for production.                                 ---
  -------------------------------------------------------
  \n"
  )
  set(FP_MODEL_ID "0x4A436B")
  set(FP_ANTI_SPOOFING_KEY "rie10A7ONqwd77VmkxGsblPUbMt384qjDgcEJ/ctT9Y=")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(locator_tag)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/factory_reset.c
  src/fp_adv.c
)

if (NOT CONFIG_BT_FAST_PAIR_FMDN_RING_COMP_NONE)
  target_sources(app PRIVATE src/ring.c)
endif()

target_include_directories(app PRIVATE include)

add_subdirectory(src/ui)
target_link_libraries(app PRIVATE app_ui)

add_subdirectory(src/battery)
target_link_libraries(app PRIVATE app_battery)
# NORDIC SDK APP END
