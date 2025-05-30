#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(periph_stress)

FILE(GLOB app_sources src/*.c)

zephyr_include_directories_ifdef(CONFIG_DT_HAS_SNPS_DWC2_ENABLED ${ZEPHYR_BASE}/samples/subsys/usb/common)
zephyr_sources_ifdef(CONFIG_DT_HAS_SNPS_DWC2_ENABLED ${ZEPHYR_BASE}/samples/subsys/usb/common/sample_usbd_init.c)

target_sources(app PRIVATE ${app_sources})
