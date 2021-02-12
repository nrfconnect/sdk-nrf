.. _bt_mesh_scene_cli_readme:

Scene Client
############

.. contents::
   :local:
   :depth: 2

The Scene Client model remotely controls the state of a :ref:`bt_mesh_scene_srv_readme` model.

Unlike the Scene Server model, the Scene Client only creates a single model instance in the mesh composition data.
The Scene Client can send messages to both the Scene Server and the Scene Setup Server, as long as it has the right application keys.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/scene_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/scene_cli.c`

.. doxygengroup:: bt_mesh_scene_cli
   :project: nrf
   :members:
