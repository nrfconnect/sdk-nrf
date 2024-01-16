#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Set Kconfig root files that will be processed as a first Kconfig for used child images.
set(mcuboot_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.mcuboot.root")
set(multiprotocol_rpmsg_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.multiprotocol_rpmsg.root")
set(hci_ipc_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.hci_ipc.root")

# For prj.conf the CONF_FILE is empty. In other case extract the exact file name from the path string.
if(CONF_FILE)
    get_filename_component(CONFIG_FILE_NAME ${CONF_FILE} NAME)
endif()

if(NOT "${CONFIG_FILE_NAME}" STREQUAL "prj_no_dfu.conf")
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_dfu.yml)
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-template)

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
    src/main.cpp
    src/zap-generated/IMClusterCommandHandler.cpp
    src/zap-generated/callback-stub.cpp
    ${COMMON_ROOT}/src/board/led_widget.cpp
    ${COMMON_ROOT}/src/app/task_executor.cpp
    ${COMMON_ROOT}/src/board/board.cpp
    ${COMMON_ROOT}/src/app/matter_init.cpp
    ${COMMON_ROOT}/src/app/matter_event_handler.cpp
)

if(CONFIG_CHIP_OTA_REQUESTOR)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/dfu/ota/ota_util.cpp)
endif()

if(CONFIG_NCS_SAMPLE_MATTER_OPERATIONAL_KEYS_MIGRATION_TO_ITS)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/migration/migration_manager.cpp)
endif()

chip_configure_data_model(app
    INCLUDE_SERVER
    BYPASS_IDL
    GEN_DIR src/zap-generated
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/template.zap
)
# NORDIC SDK APP END
