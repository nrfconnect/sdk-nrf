.. _bt_mesh_light_ctrl_cli_readme:

Light Lightness Control Client
##############################

.. contents::
   :local:
   :depth: 2

The Light Lightness Control (LC) Client configures and interacts with the :ref:`bt_mesh_light_ctrl_srv_readme`.

The Light LC Client creates a single model instance in the mesh composition data, and it can send messages to both the Light LC Server and the Light LC Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctrl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctrl_cli.c`

.. doxygengroup:: bt_mesh_light_ctrl_cli
   :project: nrf
   :members:
