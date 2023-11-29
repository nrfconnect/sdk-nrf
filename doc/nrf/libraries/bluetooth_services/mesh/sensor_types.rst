.. _bt_mesh_sensor_types_readme:

Bluetooth Mesh sensor types
###########################

.. contents::
   :local:
   :depth: 2

All sensor types are collected in :file:`include/bluetooth/mesh/sensor_types.h`, and are divided into the categories listed in the page index.

To keep the total flash usage down, the sensor types are only instantiated if they're referenced by the application.
This behavior can be overridden by enabling :kconfig:option:`CONFIG_BT_MESH_SENSOR_ALL_TYPES`.
Note that if the Sensor Client is enabled, :kconfig:option:`CONFIG_BT_MESH_SENSOR_ALL_TYPES` is enabled by default.

Sensor types can be forced into the build by the :c:macro:`BT_MESH_SENSOR_TYPE_FORCE` macro.

Sensor types may only be declared in the ``bt_mesh_sensor_types`` static linker section.

See :ref:`bt_mesh_sensor_types` for information on how to use these sensor types when initializing and using sensors.

.. doxygengroup:: bt_mesh_sensor_types
   :project: nrf
   :content-only:

.. _bt_mesh_sensor_types_occupancy_readme:

Occupancy sensor types
**********************

.. doxygengroup:: bt_mesh_sensor_types_occupancy
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_ambient_temperature_readme:

Ambient temperature sensor types
********************************

.. doxygengroup:: bt_mesh_sensor_types_ambient_temperature
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_environmental_readme:

Environmental sensor types
**************************

.. doxygengroup:: bt_mesh_sensor_types_environmental
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_device_operating_temperature_readme:

Device operating temperature sensor types
*****************************************

.. doxygengroup:: bt_mesh_sensor_types_device_operating_temperature
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_electrical_input_readme:

Electrical input sensor types
*****************************

.. doxygengroup:: bt_mesh_sensor_types_electrical_input
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_energy_management_readme:

Energy management sensor types
******************************

.. doxygengroup:: bt_mesh_sensor_types_energy_management
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_photometry_readme:

Photometry sensor types
***********************

.. doxygengroup:: bt_mesh_sensor_types_photometry
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_power_supply_output_readme:

Power supply output sensor types
********************************

.. doxygengroup:: bt_mesh_sensor_types_power_supply_output
   :project: nrf
   :members:
   :content-only:

.. _bt_mesh_sensor_types_warranty_and_service_readme:

Warranty and Service sensor types
*********************************

.. doxygengroup:: bt_mesh_sensor_types_warranty_and_service
   :project: nrf
   :members:
   :content-only:
