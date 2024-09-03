.. _bt_mesh_light_ctl_readme:

Light CTL models
################

.. contents::
   :local:
   :depth: 2

The Color-Tunable Light (CTL) models allow remote control and configuration of CTLs on a mesh device.

The Light CTL models can represent the lightness states on a linear or perceptually uniform lightness scale.
See :ref:`bt_mesh_lightness_readme` for details.

The Light CTL models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   light_ctl_cli.rst
   light_ctl_srv.rst
   light_temp_srv.rst


Common types
============

This section lists the types common to the Light CTL mesh models.

| Header file: :file:`include/bluetooth/mesh/light_ctl.h`

.. doxygengroup:: bt_mesh_light_ctl
   :project: nrf
   :members:
