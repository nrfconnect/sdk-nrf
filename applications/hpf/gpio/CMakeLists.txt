#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hpf_gpio)

hpf_assembly_install(app "${CMAKE_SOURCE_DIR}/src/hrt/hrt.c")

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(CONFIG_HPF_GPIO_BACKEND_ICMSG app PRIVATE src/backend/backend_icmsg.c)
target_sources_ifdef(CONFIG_HPF_GPIO_BACKEND_ICBMSG app PRIVATE src/backend/backend_icmsg.c)
target_sources_ifdef(CONFIG_HPF_GPIO_BACKEND_MBOX app PRIVATE  src/backend/backend_mbox.c)
