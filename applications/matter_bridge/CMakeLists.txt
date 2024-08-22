#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-bridge)

# Enable GNU STD support and initialize the Matter Data Model.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

target_include_directories(app PRIVATE
    src
    src/bridged_device_types
    ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/bridge/
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/bridge/util
)

target_sources(app PRIVATE
    src/app_task.cpp
    src/main.cpp
    src/bridge_shell.cpp
    src/zcl_callbacks.cpp
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/bridge/bridge_manager.cpp
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/bridge/matter_bridged_device.cpp
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/bridge/bridge_storage_manager.cpp
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/bridge/bridged_device_data_provider.cpp
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/binding/binding_handler.cpp
)

if(CONFIG_BRIDGED_DEVICE_BT)
    target_sources(app PRIVATE
         ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/bridge/ble_connectivity_manager.cpp
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

chip_configure_data_model(app
    INCLUDE_SERVER
    BYPASS_IDL
    GEN_DIR ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/zap-generated
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/bridge.zap
)
# NORDIC SDK APP END
