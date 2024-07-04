#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(broadcaster_and_central)

target_sources(app PRIVATE
    main.c
    ../modules/iso_broadcast_src.c
    ../modules/iso_broadcast_sink.c
    ../modules/acl_central.c
    ../modules/acl_peripheral.c)
zephyr_include_directories(../modules)
