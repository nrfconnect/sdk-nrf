#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT SYSBUILD)
	message(FATAL_ERROR
		" This is a multi-image application that should be built using sysbuild.\n"
		" Add --sysbuild argument to west build command to prepare all the images.")
endif()

project(idle_usb)

zephyr_include_directories($ENV{ZEPHYR_BASE}/samples/subsys/usb/common)
zephyr_sources($ENV{ZEPHYR_BASE}/samples/subsys/usb/common/sample_usbd_init.c)
target_sources(app PRIVATE src/main.c)
