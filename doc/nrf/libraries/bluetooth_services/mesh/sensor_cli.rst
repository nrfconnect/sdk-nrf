.. _bt_mesh_sensor_cli_readme:

Sensor Client
#############

.. contents::
   :local:
   :depth: 2

The Sensor Client model reads and configures the sensors exposed by :ref:`bt_mesh_sensor_srv_readme` models.

Unlike the Sensor Server model, the Sensor Client only creates a single model instance in the mesh composition data.
The Sensor Client can send messages to both the Sensor Server and the Sensor Setup Server, as long as it has the right application keys.

Configuration
=============

The Sensor Client will receive data from the Sensor Servers asynchronously.
In most cases, the incoming data needs to be interpreted differently based on the type of sensor that produced it.

When parsing incoming messages, the Sensor Client is required to do a lookup of the sensor type of the data so it can decode it correctly.
For this purpose, the :ref:`bt_mesh_sensor_types` module includes the :ref:`list of available sensor types <bt_mesh_sensor_types_readme>`.

By default, all sensor types are available when the Sensor Client model is compiled in, but this behavior can be overridden to reduce flash consumption by explicitly disabling :option:`CONFIG_BT_MESH_SENSOR_ALL_TYPES`.
In this case, only the referenced sensor types will be available.

.. note::
    Whenever the Sensor Client receives a sensor type that it is unable to interpret, it calls its :c:member:`bt_mesh_sensor_cli_handlers.unknown_type` callback.
    The Sensor Client API is designed to force the application to reference any sensor types it wants to communicate with, so this issue will commonly not occur.

The Sensor Client API supports both blocking functions and asynchronous callbacks for accessing the Sensor Server data.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/sensor_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/sensor_cli.c`

.. doxygengroup:: bt_mesh_sensor_cli
   :project: nrf
   :members:
