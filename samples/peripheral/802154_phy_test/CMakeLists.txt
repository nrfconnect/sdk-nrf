#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

list(APPEND EXTRA_ZEPHYR_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules/app_rpc)
list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules/app_rpc/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(phy_tt LANGUAGES NONE)

target_include_directories(app PRIVATE
    include
    include/internal
    include/internal/ctrl
    include/internal/rf)

target_sources(app PRIVATE
    src/main.c
    src/comm_proc.c
    src/rf_proc.c
    src/ptt_conf.c
    src/periph_proc.c
    src/timer_proc.c
    src/ctrl/ptt_ctrl.c
    src/ctrl/ptt_events.c
    src/ctrl/ptt_mode_manager.c
    src/ctrl/ptt_parser.c
    src/ctrl/ptt_proto.c
    src/ctrl/ptt_rf_proc.c
    src/ctrl/ptt_timers.c
    src/ctrl/ptt_uart_proc.c
    src/ctrl/ptt_zb_perf_cmd_mode_ota.c
    src/ctrl/ptt_zb_perf_cmd_mode_response.c
    src/ctrl/ptt_zb_perf_cmd_mode_uart.c
    src/ctrl/ptt_zb_perf_cmd_mode.c
    src/ctrl/ptt_zb_perf_dut_mode.c
    src/rf/ptt_rf.c)
