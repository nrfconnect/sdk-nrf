#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(matter-lock)

# Enable GNU STD support and initialize the Matter Data Model.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

target_include_directories(app PRIVATE
    src
)

SET(access_srcs src/access/access_data_types.cpp
              src/access/access_manager_credentials.cpp
              src/access/access_manager_users.cpp
              src/access/access_manager.cpp
              src/access/access_storage.cpp
              )

if(CONFIG_LOCK_SCHEDULES)
    LIST(APPEND access_srcs src/access/access_manager_schedules.cpp)
endif()

target_sources(app PRIVATE
    src/app_task.cpp
    src/bolt_lock_manager.cpp
    src/main.cpp
    src/zcl_callbacks.cpp
    ${access_srcs}
)

if(CONFIG_THREAD_WIFI_SWITCHING)
    target_sources(app PRIVATE
        src/thread_wifi_switch.cpp
    )
endif()

chip_configure_data_model(app
    INCLUDE_SERVER
    BYPASS_IDL
    GEN_DIR src/zap-generated
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/lock.zap
)
# NORDIC SDK APP END
