#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(chip-light-bulb)

set(COMMON_ROOT ${CMAKE_CURRENT_LIST_DIR}/../common)

# NORDIC SDK APP START
target_include_directories(app PRIVATE src ${COMMON_ROOT}/src)

target_sources(app PRIVATE
               src/app_task.cpp
               src/lighting_manager.cpp
               src/light_bulb_publish_service.cpp
               src/gen/call-command-handler.cpp
               src/gen/callback-stub.cpp
               src/main.cpp
               src/zcl_callbacks.cpp
               ${COMMON_ROOT}/src/led_widget.cpp
               ${COMMON_ROOT}/src/nfc_widget.cpp
               ${COMMON_ROOT}/src/thread_util.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/server/DataModelHandler.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/reporting/reporting-default-configuration.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/reporting/reporting.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/af-event.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/af-main-common.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/attribute-size.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/attribute-storage.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/attribute-table.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/binding-table.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/chip-message-send.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/client-api.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/ember-print.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/message.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/process-cluster-message.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/process-global-message.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/util.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/on-off-server/on-off.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/level-control/level-control.cpp)
# NORDIC SDK APP END
