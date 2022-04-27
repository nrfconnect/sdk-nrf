#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(APPLICATION_CONFIG_DIR "configuration/\${BOARD}")

if(NOT CONF_FILE STREQUAL "prj_no_dfu.conf")
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_dfu.yml)
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-light-switch)

set(COMMON_ROOT ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common)
set(NLIO_ROOT ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/third_party/nlio/repo)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

# NORDIC SDK APP START
target_include_directories(app PRIVATE
    src
    ${COMMON_ROOT}/src
    ${NLIO_ROOT}/include
    ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/zzz_generated/app-common
)

target_compile_options(app PRIVATE -Wno-deprecated-declarations)

target_sources(app PRIVATE
    src/app_task.cpp
    src/light_switch.cpp
    src/main.cpp
    src/binding_handler.cpp
    src/shell_commands.cpp
    ${COMMON_ROOT}/src/led_widget.cpp
    ${COMMON_ROOT}/src/thread_util.cpp
    src/zap-generated/IMClusterCommandHandler.cpp
    src/zap-generated/callback-stub.cpp
)

if(CONFIG_CHIP_OTA_REQUESTOR)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/ota_util.cpp)
endif()

if(CONFIG_MCUMGR_SMP_BT)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/dfu_over_smp.cpp)
endif()

chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/light_switch.zap
)
# NORDIC SDK APP END
