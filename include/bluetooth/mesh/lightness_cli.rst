.. _bt_mesh_lightness_cli_readme:

Light Lightness Client
######################

.. contents::
   :local:
   :depth: 2

The Light Lightness Client model remotely controls the state of a :ref:`bt_mesh_lightness_srv_readme` model.

Unlike the Light Lightness Server model, the Light Lightness Client only creates a single model instance in the mesh composition data.
The Light Lightness Client can send messages to both the Light Lightness Server and the Light Lightness Setup Server, as long as it has the right application keys.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/lightness_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/lightness_cli.c`

.. doxygengroup:: bt_mesh_lightness_cli
   :project: nrf
   :members:
