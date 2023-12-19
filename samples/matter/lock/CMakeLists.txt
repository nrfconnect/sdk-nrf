#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Set Kconfig root files that will be processed as a first Kconfig for used child images.
set(mcuboot_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.mcuboot.root")
set(multiprotocol_rpmsg_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.multiprotocol_rpmsg.root")
set(hci_rpmsg_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.hci_rpmsg.root")

# For prj.conf the CONF_FILE is empty. In other case extract the exact file name from the path string.
if(CONF_FILE)
    get_filename_component(CONF_FILE_NAME ${CONF_FILE} NAME CACHE)
endif()

if(NOT CONF_FILE_NAME STREQUAL "prj_no_dfu.conf")
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_dfu.yml)
endif()

if(CONF_FILE_NAME STREQUAL "prj_thread_wifi_switched.conf")
    # TODO: Add Kconfig for adding custom Matter compilation flags
    set(CHIP_CFLAGS "-flto")
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-lock)

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
    src/bolt_lock_manager.cpp
    src/main.cpp
    src/zcl_callbacks.cpp
    src/zap-generated/callback-stub.cpp
    src/zap-generated/IMClusterCommandHandler.cpp
    ${COMMON_ROOT}/src/led_widget.cpp
    ${COMMON_ROOT}/src/task_executor.cpp
    ${COMMON_ROOT}/src/board.cpp
    ${COMMON_ROOT}/src/matter_init.cpp
)

if(CONFIG_CHIP_OTA_REQUESTOR OR CONFIG_MCUMGR_TRANSPORT_BT)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/ota_util.cpp)
endif()

if(CONFIG_CHIP_NUS)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/bt_nus_service.cpp)
endif()

if(CONFIG_MCUMGR_TRANSPORT_BT)
    zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/dfu_over_smp.cpp)
endif()

if(CONFIG_THREAD_WIFI_SWITCHING)
    target_sources(app PRIVATE src/thread_wifi_switch.cpp)

    # Enable link-time optimization for the app and WPA supplicant libraries
    target_compile_options(app PRIVATE -flto)
    target_compile_options(..__nrf__modules__hostap PRIVATE -flto)
endif()

chip_configure_data_model(app
    INCLUDE_SERVER
    BYPASS_IDL
    GEN_DIR src/zap-generated
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/lock.zap
)
# NORDIC SDK APP END
