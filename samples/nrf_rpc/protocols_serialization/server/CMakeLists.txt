#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# # Do not include obfuscated symbols in case of building from sources
if (CONFIG_MPSL_BUILD_TYPE_SRC_DEBUG)
  target_compile_definitions(nrf-802154-driver-interface INTERFACE OBFUSCATE_LIBMPSL_PROTOCOL_DEFINES_H__=1)
endif()

project(protocols_serialization_server)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

if(CONFIG_NRF_PS_SERVER_FATAL_ERROR_TRIGGER)
  target_sources(app PRIVATE src/fatal_error_trigger.c)
endif()


if(CONFIG_NRF_PS_SERVER_HEALTH_MONITOR)
  target_sources(app PRIVATE src/hot_main.c)
endif()

if(CONFIG_NRF_PS_SERVER_RPC_ALIVE_LED)
  # Wrap z_fatal_error to turn off the RPC alive LED on fatal error
  target_link_options(app INTERFACE -Wl,--wrap=z_fatal_error)
endif()
# NORDIC SDK APP END

# Link OpenThread CLI even though OPENTHREAD_SHELL is not selected
if(CONFIG_OPENTHREAD_RPC)
  target_link_libraries(zephyr PRIVATE openthread-cli-ftd)
endif()
