#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf_rpc_remote)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Link OpenThread CLI even though OPENTHREAD_SHELL is not selected
#zephyr_link_libraries(openthread-cli-ftd)
