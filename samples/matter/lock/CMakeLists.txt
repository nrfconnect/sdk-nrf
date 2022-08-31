#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

get_filename_component(MATTER_MODULE_ROOT $ENV{ZEPHYR_BASE}/../modules/lib/matter REALPATH)

# Set Kconfig root files that will be processed as a first Kconfig for used child images.
set(mcuboot_KCONFIG_ROOT ${MATTER_MODULE_ROOT}/config/nrfconnect/chip-module/Kconfig.mcuboot.root)
set(multiprotocol_rpmsg_KCONFIG_ROOT ${MATTER_MODULE_ROOT}/config/nrfconnect/chip-module/Kconfig.multiprotocol_rpmsg.root)
set(hci_rpmsg_KCONFIG_ROOT ${MATTER_MODULE_ROOT}/config/nrfconnect/chip-module/Kconfig.hci_rpmsg.root)

if(NOT CONF_FILE STREQUAL "prj_no_dfu.conf")
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_dfu.yml)
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
    src/zap-generated/IMClusterCommandHandler.cpp
    src/zap-generated/callback-stub.cpp
    ${COMMON_ROOT}/src/led_widget.cpp
    $<$<BOOL:${CONFIG_NET_L2_OPENTHREAD}>:${COMMON_ROOT}/src/thread_util.cpp>
)

if(CONFIG_CHIP_OTA_REQUESTOR)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/ota_util.cpp)
endif()

if(CONFIG_MCUMGR_SMP_BT)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/dfu_over_smp.cpp)
endif()

chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/lock.zap
)
# NORDIC SDK APP END
