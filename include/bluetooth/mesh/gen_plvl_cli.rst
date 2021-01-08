.. _bt_mesh_plvl_cli_readme:

Generic Power Level Client
##########################

.. contents::
   :local:
   :depth: 2

The Generic Power Level Client model remotely controls the state of a Generic Power Level Server model.

Unlike the Generic Power Level Server model, the Generic Power Level Client only creates a single model instance in the mesh composition data.
The Generic Power Level Client may send messages to both the Generic Power Level Server and the Generic Power Level Setup Server, as long as it has the right application keys.

Extended models
================

None.

Persistent storage
===================

None.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/gen_plvl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_plvl_cli.c`

.. doxygengroup:: bt_mesh_plvl_cli
   :project: nrf
   :members:
