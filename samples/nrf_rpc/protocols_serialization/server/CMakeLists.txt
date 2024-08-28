#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(protocols_serialization_server)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

if(CONFIG_NRF_PS_SERVER_FATAL_ERROR_TRIGGER)
    target_sources(app PRIVATE src/fatal_error_trigger.c)
endif()
# NORDIC SDK APP END

# Link OpenThread CLI even though OPENTHREAD_SHELL is not selected
if(CONFIG_OPENTHREAD_RPC)
    zephyr_link_libraries(openthread-cli-ftd)
endif()
