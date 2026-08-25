#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hci_vs_fatal_error)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/ipc_radio/src
)

target_sources(app PRIVATE src/main.c)
