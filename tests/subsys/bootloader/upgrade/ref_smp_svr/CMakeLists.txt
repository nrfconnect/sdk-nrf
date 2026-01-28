#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(smp_svr_ref)

set(smp_srv_dir ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr)

# This project uses orginal sdk-zephyr C source code
target_sources(app PRIVATE ${smp_srv_dir}/src/main.c)
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE ${smp_srv_dir}/src/bluetooth.c)

zephyr_get(PERIPHCONF_BIN)
if (DEFINED PERIPHCONF_BIN)
  target_sources(app PRIVATE periphconf_check.c)
  message("Including local periphconf '${PERIPHCONF_BIN}'")
  generate_inc_file_for_target(app ${PERIPHCONF_BIN} ${ZEPHYR_BINARY_DIR}/include/generated/periphconf.bin.inc)
endif()
