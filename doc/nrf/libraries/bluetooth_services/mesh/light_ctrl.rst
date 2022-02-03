.. _bt_mesh_light_ctrl_readme:

Light Lightness Control models
##############################

.. contents::
   :local:
   :depth: 2

The Light Lightness Control models implement common energy saving behavior for light fixtures by taking control of the Light Lightness Server.
While :ref:`bt_mesh_lightness_srv_readme` instances can be operated directly by dimmers and light switches, these devices are not time-aware, and cannot provide any dynamic behavior without extensive application logic on top.
By adding an inactivity timer and a regulator for the ambient light level, the Light Lightness Control models make light fixtures behave dynamically and conserve energy.

The dynamic behavior is implemented by the server, while the client allows the configuration of the server's parameters.

The Light Lightness Control mesh models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   light_ctrl_srv.rst
   light_ctrl_cli.rst
   light_ctrl_reg.rst
   light_ctrl_reg_spec.rst



.. _bt_mesh_light_ctrl_common:

Common types
************

This section lists the types common to the Light Lightness Control mesh models.

| Header file: :file:`include/bluetooth/mesh/light_ctrl.h`

.. doxygengroup:: bt_mesh_light_ctrl
   :project: nrf
   :members:
