#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

option(BUILD_WITH_DFU "Build target with Device Firmware Upgrade support" OFF)
if(BUILD_WITH_DFU)
# Use multi-image DFU for the nRF5340dk
    if(${BOARD} STREQUAL "nrf5340dk_nrf5340_cpuapp")
        set(CONF_FILE prj_multi_image_dfu.conf)
    else()
        set(CONF_FILE prj_single_image_dfu.conf)
    endif()
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml")
        set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml)
    else()
        message(FATAL_ERROR "The ${BOARD} target does not support DFU")
    endif()
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-light-bulb)

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

target_sources(app PRIVATE
    src/app_task.cpp
    src/lighting_manager.cpp
    src/light_bulb_publish_service.cpp
    src/main.cpp
    src/zcl_callbacks.cpp
    src/zap-generated/attribute-size.cpp
    src/zap-generated/IMClusterCommandHandler.cpp
    src/zap-generated/callback-stub.cpp
    ${COMMON_ROOT}/src/led_widget.cpp
    ${COMMON_ROOT}/src/thread_util.cpp
)

if(BUILD_WITH_DFU)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/dfu_over_smp.cpp)
endif()

chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/light_bulb.zap
)
# NORDIC SDK APP END
