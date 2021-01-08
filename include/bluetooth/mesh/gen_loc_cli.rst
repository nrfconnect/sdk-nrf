.. _bt_mesh_loc_cli_readme:

Generic Location Client
#######################

.. contents::
   :local:
   :depth: 2

The Generic Location Client model can get and set the state of a :ref:`bt_mesh_loc_srv_readme` model remotely.

Unlike the Generic Location Server model, the Generic Location Client only creates a single model instance in the mesh composition data.
The Generic Location Client may send messages to both the Generic Location Server and the Generic Location Setup Server, as long as it has the right application keys.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_loc_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_loc_cli.c`

.. doxygengroup:: bt_mesh_loc_cli
   :project: nrf
   :members:
