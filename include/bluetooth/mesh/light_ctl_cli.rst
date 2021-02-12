.. _bt_mesh_light_ctl_cli_readme:

Light CTL Client
################

.. contents::
   :local:
   :depth: 2

The Color-Tunable Light (CTL) Client model remotely controls the state of the :ref:`bt_mesh_light_ctl_srv_readme` model, and the :ref:`bt_mesh_light_temp_srv_readme` model.

Unlike the server models, the Light CTL Client only creates a single model instance in the mesh composition data.
The Light CTL Client can send messages to the Light CTL Server, the Light CTL Setup Server and the Light CTL Temperature Server, as long as it has the right application keys.

.. note::
   By specification, the Light CTL Temperature Server must be located in another element than the Light CTL Server models.
   The user must ensure that the publish parameters of the client instance point to the correct element address when using the function calls in the client API.

Extended models
===============

None.

Persistent storage
==================

None.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/light_ctl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctl_cli.c`

.. doxygengroup:: bt_mesh_light_ctl_cli
   :project: nrf
   :members:
