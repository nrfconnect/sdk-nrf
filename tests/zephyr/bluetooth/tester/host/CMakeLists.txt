#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(host_tester)

zephyr_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/host)

target_sources(app PRIVATE
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/main.c
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp.c
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp_core.c
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp_gap.c
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp_gatt.c
)

if(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
  target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp_l2cap.c)
endif()

if(CONFIG_BT_IAS OR CONFIG_BT_IAS_CLIENT)
  target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp_ias.c)
endif()

if(CONFIG_BT_OTS)
  target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp_ots.c)
endif()

zephyr_include_directories(
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/btp
  ${ZEPHYR_BASE}/tests/bluetooth/tester/src/audio/btp
)
