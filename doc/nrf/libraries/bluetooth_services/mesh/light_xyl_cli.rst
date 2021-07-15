.. _bt_mesh_light_xyl_cli_readme:

Light xyL Client
################

.. contents::
   :local:
   :depth: 2

The xyl Client model remotely controls the state of the :ref:`bt_mesh_light_xyl_srv_readme` model.

Unlike the server model, the client only creates a single model instance in the mesh composition data.
The Light xyL Client can send messages to the Light xyL Server and the Light xyL Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_xyl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_xyl_cli.c`

.. doxygengroup:: bt_mesh_light_xyl_cli
   :project: nrf
   :members:
