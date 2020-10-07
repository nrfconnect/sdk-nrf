.. _bt_mesh_time_cli_readme:

Time Client
###########

The Time Client model provides time sources for and configures :ref:`bt_mesh_time_srv_readme` models.

Unlike the server model, the client only creates a single model instance in the mesh composition data.
The Time Client can send messages to both the Time Server and the Time Setup Server, as long as it has the right application keys.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/time_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/time_cli.c`

.. doxygengroup:: bt_mesh_time_cli
   :project: nrf
   :members:
