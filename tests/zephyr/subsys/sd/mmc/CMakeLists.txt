# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mmc_subsys_test)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/subsys/sd/mmc/src/main.c)
