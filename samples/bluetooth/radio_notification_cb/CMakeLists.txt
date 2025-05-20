#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

zephyr_sources_ifdef(CONFIG_BT_CENTRAL src/central.c)
zephyr_sources_ifdef(CONFIG_BT_PERIPHERAL src/peripheral.c)

target_sources(app PRIVATE
  src/main.c
)
