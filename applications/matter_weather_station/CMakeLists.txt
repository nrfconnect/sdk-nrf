#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ZDebug)
endif()

set(CONF_FILE           ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/prj_${CMAKE_BUILD_TYPE}.conf)
set(PM_STATIC_YML_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-weather-station)

set(COMMON_ROOT ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common)
set(NLIO_ROOT ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/third_party/nlio/repo)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)

# NORDIC SDK APP START
target_include_directories(app PRIVATE src ${COMMON_ROOT}/src ${NLIO_ROOT}/include)

target_sources(app PRIVATE
               src/app_task.cpp
               src/main.cpp
               src/gen/attribute-size.cpp
               src/gen/IMClusterCommandHandler.cpp
               src/gen/callback-stub.cpp
               ${COMMON_ROOT}/src/thread_util.cpp
               ${COMMON_ROOT}/src/led_widget.cpp
               ${COMMON_ROOT}/src/dfu_over_smp.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/DataModelHandler.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/reporting/reporting-default-configuration.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/reporting/reporting.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/af-event.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/af-main-common.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/attribute-list-byte-span.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/attribute-size-util.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/attribute-storage.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/attribute-table.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/binding-table.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/chip-message-send.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/client-api.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/ember-compatibility-functions.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/ember-print.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/error-mapping.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/message.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/process-cluster-message.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/process-global-message.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/util.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/basic/basic.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/bindings/bindings.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/general-commissioning-server/general-commissioning-server.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/network-commissioning/network-commissioning-ember.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/network-commissioning/network-commissioning.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/clusters/operational-credentials-server/operational-credentials-server.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/server/EchoHandler.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/server/Mdns.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/server/OnboardingCodesUtil.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/server/RendezvousServer.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/server/Server.cpp
               ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/server/StorablePeerConnection.cpp)
# NORDIC SDK APP END
