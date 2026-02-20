#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(host_tester)

zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host/host)

target_sources(app PRIVATE
    src/main.c
    src/btp.c
    src/btp_core.c
    src/btp_gap.c
    src/btp_gatt.c
    )
if(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
  target_sources(app PRIVATE src/btp_l2cap.c)
endif()

if(CONFIG_BT_IAS OR CONFIG_BT_IAS_CLIENT)
  target_sources(app PRIVATE src/btp_ias.c)
endif()

if(CONFIG_BT_OTS)
  target_sources(app PRIVATE src/btp_ots.c)
endif()

if(CONFIG_BT_CLASSIC)
  target_sources(app PRIVATE src/btp_sdp.c)
endif()

zephyr_include_directories(
   src
   src/btp
   src/audio/btp
)
