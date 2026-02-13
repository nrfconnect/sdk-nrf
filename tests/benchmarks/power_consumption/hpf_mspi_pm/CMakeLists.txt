#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hpf_mspi_pm)

target_sources(app PRIVATE ./../common/main.c)
target_sources(app PRIVATE src/hpf_mspi_pm.c)
