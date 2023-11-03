#
# Copyright (c) 2022 Nordic Semiconductor
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
  set(FP_MODEL_ID "0x2A410B")
  set(FP_ANTI_SPOOFING_KEY "Unoh+nycK/ZJ7k3dHsdcNpiP1SfOy0P/Lx5XixyYois=")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(input_device)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/bt_adv_helper.c
  src/hids_helper.c
  src/bt_le_adv_prov_uuid16.c
  src/battery_module.c
)

target_include_directories(app PRIVATE include)
# NORDIC SDK APP END
