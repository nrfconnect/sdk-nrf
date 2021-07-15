.. _bt_mesh_light_hsl_cli_readme:

Light HSL Client
################

.. contents::
   :local:
   :depth: 2

The Light HSL Client model remotely controls the state of the :ref:`bt_mesh_light_hsl_srv_readme`, :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` models.

Unlike the Light HSL Server model, the Client only creates a single model instance in the mesh composition data.

Extended models
***************

None.

Persistent storage
******************

None.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_hsl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_hsl_cli.c`

.. doxygengroup:: bt_mesh_light_hsl_cli
   :project: nrf
   :members:
