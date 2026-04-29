#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ab_split)

set(ab_split_sample_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/dfu/ab_split)

target_sources(app PRIVATE
  ${ab_split_sample_dir}/src/main.c
  ${ab_split_sample_dir}/src/ab_utils.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/include
  ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/include
  ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src
  ${ab_split_sample_dir}/src
)

target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE
  ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c
)
