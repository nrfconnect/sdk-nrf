#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Set Kconfig root files that will be processed as a first Kconfig for used child images.
set(mcuboot_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.mcuboot.root")
set(hci_ipc_KCONFIG_ROOT "\\\${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/Kconfig.hci_ipc.root")

# For prj.conf the CONF_FILE is empty. In other case extract the exact file name from the path string.
if(CONF_FILE)
    get_filename_component(CONFIG_FILE_NAME ${CONF_FILE} NAME)
endif()

if(NOT CONFIG_FILE_NAME STREQUAL "prj_no_dfu.conf")
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_dfu.yml)
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-bridge)

set(COMMON_ROOT ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common)
set(NLIO_ROOT ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/third_party/nlio/repo)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

# NORDIC SDK APP START
target_include_directories(app PRIVATE
    src
    src/bridged_device_types
    ${COMMON_ROOT}/src
    ${COMMON_ROOT}/src/bridge/
    ${COMMON_ROOT}/src/bridge/util
    ${NLIO_ROOT}/include
    ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/zzz_generated/app-common
)

target_sources(app PRIVATE
    src/app_task.cpp
    src/main.cpp
    src/bridge_shell.cpp
    ${COMMON_ROOT}/src/bridge/bridge_manager.cpp
    ${COMMON_ROOT}/src/bridge/matter_bridged_device.cpp
    ${COMMON_ROOT}/src/bridge/bridge_storage_manager.cpp
    ${COMMON_ROOT}/src/bridge/bridged_device_data_provider.cpp
    ${COMMON_ROOT}/src/binding/binding_handler.cpp
    src/zap-generated/IMClusterCommandHandler.cpp
    src/zap-generated/callback-stub.cpp
    ${COMMON_ROOT}/src/led/led_widget.cpp
    ${COMMON_ROOT}/src/ps_storage/persistent_storage_util.cpp
    ${COMMON_ROOT}/src/tasks/task_executor.cpp
    ${COMMON_ROOT}/src/board/board.cpp
    ${COMMON_ROOT}/src/init/matter_init.cpp
)

if(CONFIG_BRIDGED_DEVICE_BT)
    target_sources(app PRIVATE
        ${COMMON_ROOT}/src/bridge/ble_connectivity_manager.cpp
        src/ble_providers/ble_bridged_device_factory.cpp
    )
    target_include_directories(app PRIVATE src/ble_providers)

if(CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE AND (CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE OR CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE))
    target_sources(app PRIVATE
        src/bridged_device_types/onoff_light.cpp
        src/ble_providers/ble_lbs_data_provider.cpp
    )

if(CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE)
    target_sources(app PRIVATE src/bridged_device_types/generic_switch.cpp)
endif()

if(CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE)
    target_sources(app PRIVATE
        src/bridged_device_types/onoff_light_switch.cpp
    )
endif()

endif() # CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE AND (CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE OR CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE)

if(CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE)
    target_sources(app PRIVATE
        src/bridged_device_types/humidity_sensor.cpp
    )
endif() # CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE

if(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE)
    target_sources(app PRIVATE
        src/bridged_device_types/temperature_sensor.cpp
    )
endif() # CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE

# Assume it makes no sense to support BLE environmental sensor without all Matter counterparts
if(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE AND CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE)
    target_sources(app PRIVATE
        src/ble_providers/ble_environmental_data_provider.cpp
    )
endif() # CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE AND CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE

else()
    target_sources(app PRIVATE
        src/simulated_providers/simulated_bridged_device_factory.cpp
    )
    target_include_directories(app PRIVATE src/simulated_providers)

if(CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE AND (CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE OR CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE))
    target_sources(app PRIVATE
        src/bridged_device_types/onoff_light.cpp
        src/simulated_providers/simulated_onoff_light_data_provider.cpp
    )

if(CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE)
    target_sources(app PRIVATE
        src/bridged_device_types/generic_switch.cpp
        src/simulated_providers/simulated_generic_switch_data_provider.cpp
    )
endif()

if(CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE)
    target_sources(app PRIVATE
        src/bridged_device_types/onoff_light_switch.cpp
        src/simulated_providers/simulated_onoff_light_switch_data_provider.cpp
    )
endif()

endif() # CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE AND (CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE OR CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE))

if(CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE)
    target_sources(app PRIVATE
            src/bridged_device_types/temperature_sensor.cpp
            src/simulated_providers/simulated_temperature_sensor_data_provider.cpp
    )
endif() # CONFIG_BRIDGE_TEMPERATURE_SENSOR_BRIDGED_DEVICE

if(CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE)
    target_sources(app PRIVATE
        src/bridged_device_types/humidity_sensor.cpp
        src/simulated_providers/simulated_humidity_sensor_data_provider.cpp
    )
endif() # CONFIG_BRIDGE_HUMIDITY_SENSOR_BRIDGED_DEVICE

endif() # CONFIG_BRIDGED_DEVICE_BT

if(CONFIG_CHIP_OTA_REQUESTOR OR CONFIG_MCUMGR_TRANSPORT_BT)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/dfu/ota/ota_util.cpp)
endif()

if(CONFIG_MCUMGR_TRANSPORT_BT)
    target_sources(app PRIVATE ${COMMON_ROOT}/src/dfu/smp/dfu_over_smp.cpp)
endif()

chip_configure_data_model(app
    INCLUDE_SERVER
    BYPASS_IDL
    GEN_DIR src/zap-generated
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/bridge.zap
)
# NORDIC SDK APP END
