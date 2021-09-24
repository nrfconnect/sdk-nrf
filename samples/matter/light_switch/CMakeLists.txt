#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-light-switch)

set(COMMON_ROOT ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common)
set(NLIO_ROOT ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/third_party/nlio/repo)
set(DATA_MODEL_ROOT ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/zzz_generated)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

# NORDIC SDK APP START
target_include_directories(app PRIVATE
    src
    ${COMMON_ROOT}/src
    ${NLIO_ROOT}/include
    ${DATA_MODEL_ROOT}
    ${DATA_MODEL_ROOT}/app-common
    ${DATA_MODEL_ROOT}/controller-clusters
)

target_compile_options(app PRIVATE -Wno-deprecated-declarations)

target_sources(app PRIVATE
    src/app_task.cpp
    src/light_switch.cpp
    src/main.cpp
    ${COMMON_ROOT}/src/led_widget.cpp
    ${COMMON_ROOT}/src/thread_util.cpp
    ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/util/im-client-callbacks.cpp
    ${DATA_MODEL_ROOT}/controller-clusters/zap-generated/attribute-size.cpp
    ${DATA_MODEL_ROOT}/controller-clusters/zap-generated/callback-stub.cpp
    ${DATA_MODEL_ROOT}/controller-clusters/zap-generated/CHIPClientCallbacks.cpp
    ${DATA_MODEL_ROOT}/controller-clusters/zap-generated/CHIPClusters.cpp
    ${DATA_MODEL_ROOT}/controller-clusters/zap-generated/IMClusterCommandHandler.cpp
)

chip_configure_data_model(app)
# NORDIC SDK APP END
