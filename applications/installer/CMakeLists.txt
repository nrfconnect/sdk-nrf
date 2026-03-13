#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(installer)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/include
  ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/include
)

if(CONFIG_INSTALLER_FW_LOADER_ENTRANCE_BOOT_REQ)
  zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()
