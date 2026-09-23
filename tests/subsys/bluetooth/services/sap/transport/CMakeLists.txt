# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_sap_transport)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/services/sap
)
