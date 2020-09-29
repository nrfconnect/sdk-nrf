.. _bt_mesh_light_ctl_readme:

Light CTL models
################
The Color-Tunable Light (CTL) models allow remote control and configuration of CTLs on a mesh device.

The Light CTL models can represent the lightness states on a linear or preceptually uniform lightness scale.
See :ref:`bt_mesh_lightness_readme` for details.

The following Light CTL models are supported:

.. toctree::
   :maxdepth: 1
   :glob:

   light_ctl_cli.rst
   light_ctl_srv.rst
   light_temp_srv.rst

The Light CTL models also feature their own common types, listed in the `Common types`_ section below.
For types common to all models, see :ref:`bt_mesh_models`.

Common types
============

| Header file: :file:`include/bluetooth/mesh/light_ctl.h`

.. doxygengroup:: bt_mesh_light_ctl
   :project: nrf
   :members:
