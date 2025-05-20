#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

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
