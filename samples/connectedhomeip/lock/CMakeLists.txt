#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(CHIP_ROOT ../../../../modules/lib/connectedhomeip REALPATH)
get_filename_component(COMMON_ROOT ../common REALPATH)

list(APPEND ZEPHYR_EXTRA_MODULES ${CHIP_ROOT}/config/nrfconnect/chip-module)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(chip-lock)

target_include_directories(app PRIVATE src ${COMMON_ROOT}/src)

target_sources(app PRIVATE
               src/app_task.cpp
               src/bolt_lock_manager.cpp
               src/gen/call-command-handler.cpp
               src/gen/callback-stub.cpp
               src/main.cpp
               src/zcl_callbacks.cpp
               ${COMMON_ROOT}/src/led_widget.cpp
               ${COMMON_ROOT}/src/nfc_widget.cpp
               ${COMMON_ROOT}/src/thread_util.cpp
               ${CHIP_ROOT}/src/app/reporting/reporting-default-configuration.cpp
               ${CHIP_ROOT}/src/app/reporting/reporting.cpp
               ${CHIP_ROOT}/src/app/server/DataModelHandler.cpp
               ${CHIP_ROOT}/src/app/util/af-event.cpp
               ${CHIP_ROOT}/src/app/util/af-main-common.cpp
               ${CHIP_ROOT}/src/app/util/attribute-size.cpp
               ${CHIP_ROOT}/src/app/util/attribute-storage.cpp
               ${CHIP_ROOT}/src/app/util/attribute-table.cpp
               ${CHIP_ROOT}/src/app/util/binding-table.cpp
               ${CHIP_ROOT}/src/app/util/chip-message-send.cpp
               ${CHIP_ROOT}/src/app/util/client-api.cpp
               ${CHIP_ROOT}/src/app/util/ember-print.cpp
               ${CHIP_ROOT}/src/app/util/message.cpp
               ${CHIP_ROOT}/src/app/util/process-cluster-message.cpp
               ${CHIP_ROOT}/src/app/util/process-global-message.cpp
               ${CHIP_ROOT}/src/app/util/util.cpp
               ${CHIP_ROOT}/src/app/clusters/bindings/bindings.cpp
               ${CHIP_ROOT}/src/app/clusters/on-off-server/on-off.cpp)
