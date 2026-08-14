#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tac5112_api)

# tac5112.h is a private driver header (not installed on the public
# include path); this whitebox test suite includes it directly to reach
# tac5112_reg_write()/tac5112_reg_read()/tac5112_configure_pll()/
# tac5112_dvol_from_half_db(), so add drivers/audio/ to the include path.
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/drivers/audio)

target_sources(app PRIVATE src/main.c)
