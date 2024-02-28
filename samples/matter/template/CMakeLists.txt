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

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-template)

# Enable GNU STD support and initialize the Matter Data Model.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

target_include_directories(app PRIVATE
    src
)

target_sources(app PRIVATE
    src/app_task.cpp
    src/main.cpp
)

chip_configure_data_model(app
    INCLUDE_SERVER
    BYPASS_IDL
    GEN_DIR src/zap-generated
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/template.zap
)
# NORDIC SDK APP END
