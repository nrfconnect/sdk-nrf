#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(MOSH)
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/iperf
  ${CMAKE_CURRENT_SOURCE_DIR}/ping
  ${CMAKE_CURRENT_SOURCE_DIR}/utils)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/shell.c)

add_subdirectory(src/print)
add_subdirectory(src/utils)
add_subdirectory(src/uart)
add_subdirectory(src/at)
add_subdirectory_ifdef(CONFIG_MOSH_NRF91_NON_OFFLOADING_DEV src/drivers)
add_subdirectory_ifdef(CONFIG_MOSH_WORKER_THREADS src/th)
add_subdirectory_ifdef(CONFIG_MOSH_SOCK src/sock)
add_subdirectory_ifdef(CONFIG_MOSH_LINK src/link)
add_subdirectory_ifdef(CONFIG_MOSH_PING src/ping)
add_subdirectory_ifdef(CONFIG_MOSH_GNSS src/gnss)
add_subdirectory_ifdef(CONFIG_MOSH_SMS src/sms)
add_subdirectory_ifdef(CONFIG_MOSH_LOCATION src/location)
add_subdirectory_ifdef(CONFIG_MOSH_PPP src/ppp)
add_subdirectory_ifdef(CONFIG_MOSH_FOTA src/fota)
add_subdirectory_ifdef(CONFIG_MOSH_REST src/rest)
add_subdirectory_ifdef(CONFIG_MOSH_STARTUP_CMDS src/startup_cmd)
add_subdirectory_ifdef(CONFIG_MOSH_GPIO_COUNT src/gpio_count)
add_subdirectory_ifdef(CONFIG_NRF_MODEM_LIB_TRACE src/modem_trace)
add_subdirectory(src/cloud)

zephyr_include_directories_ifdef(CONFIG_MEMFAULT config)

if(NOT ENV{PROJECT_NAME})
  zephyr_compile_definitions(
    PROJECT_NAME=${CMAKE_PROJECT_NAME}
  )
endif()
