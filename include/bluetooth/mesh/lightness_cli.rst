.. _bt_mesh_lightness_cli_readme:

Light Lightness Client
######################

The Light Lightness Client model remotely controls the state of a Light Lightness Server model.

Contrary to the Server model, the Client only creates a single model instance in the mesh composition data.
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
