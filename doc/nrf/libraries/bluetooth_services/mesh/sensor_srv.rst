.. _bt_mesh_sensor_srv_readme:

Sensor Server
#############

.. contents::
   :local:
   :depth: 2

.. note::
   A new sensor API is introduced as of |NCS| v2.6.0.
   The old API is deprecated, but still available by enabling the Kconfig option :kconfig:option:`CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE`.
   The Kconfig option is enabled by default in the deprecation period.
   See the documentation for |NCS| versions prior to v2.6.0 for documentation about the old sensor API.

The Sensor Server model holds a list of sensors, and exposes them to the mesh network.
There may be multiple Sensor Server models on a single mesh node, and each model may hold up to 47 sensors.

The Sensor Server model adds two model instances in the composition data:

* Sensor Server
* Sensor Setup Server

The two model instances share the states of the Sensor Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Sensor Server states, as the two model instances can be bound to different application keys.

* Sensor Server is the user-facing model in the pair.
  It provides access to read-only states Sensor Descriptors, Sensor Data, and Sensor Series.
* Sensor Setup Server provides access to Sensor Cadence and Sensor Settings, allowing configurator devices to set up the publish rate and parameters for each sensor.

Configuration
=============

The Sensor Server model requires a list of :c:struct:`bt_mesh_sensor` pointers at compile time.
The list of sensors cannot be changed at runtime, and only one of each type of sensors can be held by a Sensor Server.
To expose multiple sensors of the same type, multiple Sensor Servers must be instantiated on different elements.

The initialization of the Sensor Server can look like this:

.. code-block:: c

   extern struct bt_mesh_sensor temperature_sensor;
   extern struct bt_mesh_sensor motion_sensor;
   extern struct bt_mesh_sensor light_sensor;

   static struct bt_mesh_sensor* const sensors[] = {
       &temperature_sensor,
       &motion_sensor,
       &light_sensor,
   };
   static struct bt_mesh_sensor_srv sensor_srv = BT_MESH_SENSOR_SRV_INIT(sensors, ARRAY_SIZE(sensors));

Each sensor can be implemented in its own separate module by exposing the sensor instance as an external variable and pointing to it in the server's sensor list.


.. note::
    The list of pointers can be ``const``, while the sensor instances themselves cannot.

All sensors exposed by the Sensor Server must be present in the Server's list.
Passing unlisted sensor instances to the Server API results in undefined behavior.

States
======

The Sensor Server does not hold any states on its own.
Instead, it exposes the states of all its sensors.

Extended models
===============

None.

.. _bt_mesh_sensor_srv_persistent_readme:

Persistent storage
==================

The Sensor Server stores the cadence state of each sensor instance persistently, including:

* Minimum interval
* Delta thresholds
* Fast period divisor
* Fast cadence range

Any other data is managed by the application and must be stored separately.
This applies for example to sensor settings or sample data.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Sensor Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/sensor_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/sensor_srv.c`

.. doxygengroup:: bt_mesh_sensor_srv
   :project: nrf
   :members:
