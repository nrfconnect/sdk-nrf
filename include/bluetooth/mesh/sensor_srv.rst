.. _bt_mesh_sensor_srv_readme:

Sensor Server
#############

The Sensor Server model holds a list of sensors, and exposes them to the mesh network.
There may be multiple Sensor Server models on a single mesh node, and each model may hold up to 47 sensors.

The Sensor Server Model adds two model instances in the composition data:

* A Sensor Server
* A Sensor Setup Server

The two model instances share the states of the Sensor Server, but accept different messages.
This allows fine-grained control of the access rights for the Sensor Server states, as the two model instances can be bound to different application keys.

The Sensor Server is the "user facing" model in the pair, and provides access to the Sensor Descriptors, the Sensor Data and Sensor Series, all of which are read-only states.

The Sensor Setup Server provides access to the Sensor Cadence and Sensor Settings, allowing configurator devices to set up the publish rate and parameters for each sensor.

Usage
=====

The Sensor Server model requires a list of :cpp:type:`bt_mesh_sensor` pointers at compile time.
The list of sensors may not be changed at runtime, and only one of each type of sensor may be held by a Sensor Server.
To expose multiple sensors of the same type, multiple Sensor Servers must be instantiated on different elements.

Initialization of the Sensor Server may typically look like this:

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

Each sensor may be implemented in its own separate module by exposing the sensor instance as an extern variable and pointing to it in the server's sensor list.
Note that the list of pointers may be ``const``, while the sensor instances themselves may not.

All sensors exposed by the Sensor Server must be present in the Server's list.
Passing unlisted sensor instances to the Server API results in undefined behavior.

States
======

The Sensor Server does not hold any states on its own, but instead exposes the states of all its sensors.

Extended models
===============

None.

Persistent Storage
==================

The Sensor Server stores the cadence state of each sensor instance persistently, including the minimum interval, delta thresholds, fast period divisor and fast cadence range.
Any other data, such as sensor settings or sample data is managed by the application, and must be stored separately.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/sensor_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/sensor_srv.c`

.. doxygengroup:: bt_mesh_sensor_srv
   :project: nrf
   :members:
