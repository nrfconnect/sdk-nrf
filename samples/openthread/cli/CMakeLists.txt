#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(openthread_cli)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

target_sources_ifdef(CONFIG_CLI_SAMPLE_MULTIPROTOCOL app PRIVATE src/ble.c)
target_sources_ifdef(CONFIG_CLI_SAMPLE_LOW_POWER app PRIVATE src/low_power.c)


if (CONFIG_CLI_SAMPLE_MULTIPROTOCOL AND CONFIG_OPENTHREAD_BLE_TCAT)
        message( FATAL_ERROR "Multiprotocol support and TCAT functionality are not compatible." )
endif()
