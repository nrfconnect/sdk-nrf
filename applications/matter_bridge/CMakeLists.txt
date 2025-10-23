#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-bridge)

# Enable GNU STD support.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

# Include Data Model utils
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/data_model.cmake)

include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/zap_helpers.cmake)
ncs_get_zap_parent_dir(ZAP_PARENT_DIR)

target_include_directories(app PRIVATE
  src
  src/bridged_device_types
  ${ZAP_PARENT_DIR}
  src/core/util
  src/core
)

target_sources(app PRIVATE
  src/app_task.cpp
  src/main.cpp
  src/bridge_shell.cpp
  src/zcl_callbacks.cpp
  src/core/bridge_manager.cpp
  src/core/matter_bridged_device.cpp
  src/core/bridge_storage_manager.cpp
  src/core/bridged_device_data_provider.cpp
  ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/binding/binding_handler.cpp
)

if(CONFIG_BRIDGED_DEVICE_BT)
  target_sources(app PRIVATE
    src/ble/ble_connectivity_manager.cpp
    src/ble/ble_bridged_device_factory.cpp
  )
  target_include_directories(app PRIVATE
    src/ble
    src/ble/data_providers
  )

if(CONFIG_BRIDGE_ONOFF_LIGHT_BRIDGED_DEVICE AND (CONFIG_BRIDGE_GENERIC_SWITCH_BRIDGED_DEVICE OR CONFIG_BRIDGE_ONOFF_LIGHT_SWITCH_BRIDGED_DEVICE))
  target_sources(app PRIVATE
    src/bridged_device_types/onoff_light.cpp
    src/ble/data_providers/ble_lbs_data_provider.cpp
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
    src/ble/data_providers/ble_environmental_data_provider.cpp
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

ncs_configure_data_model()
# NORDIC SDK APP END
