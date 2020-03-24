.. _bt_mesh_sensor_cli_readme:

Sensor Client
#############

The Sensor Client model reads and configures the sensors exposed by Sensor Server models.

Contrary to the Server model, the Client only creates a single model instance in the mesh composition data.
The Sensor Client may send messages to both the Sensor Server and the Sensor Setup Server, as long as it has the right application keys.

Usage
=====

The Sensor Client will receive data from the Sensor Servers asynchronously, and in most cases, the incoming data needs to be interpreted differently based on the type of sensor that produced it.
When parsing incoming messages, the Sensor Client is required to do a lookup of the sensor type of the data so it can decode it correctly.
For this purpose, the list of known sensor types is centralized in the :ref:`bt_mesh_sensor_types` module.
By default, all sensor types are available when the Sensor Client model is compiled in, but this behavior can be overridden to reduce flash consumption by explicitly disabling :option:`CONFIG_BT_MESH_SENSOR_ALL_TYPES`.
In this case, only the referenced sensor types will be available.
Whenever the Sensor Client receives a sensor type it is unable to interpret, it calls its :cpp:member:`bt_mesh_sensor_cli_handlers::unknown_type` callback.
The Sensor Client API is designed to force the application to reference any sensor types it wants to communicate with, so this issue will commonly not occur.

The Sensor Client API supports both blocking functions and asynchronous callbacks for accessing the Sensor Server data.

Extended models
===============

None.

Persistent Storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/sensor_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/sensor_cli.c`

.. doxygengroup:: bt_mesh_sensor_cli
   :project: nrf
   :members:
