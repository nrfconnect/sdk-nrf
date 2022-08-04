.. _bt_mesh_lightness_readme:

Light Lightness models
######################

.. contents::
   :local:
   :depth: 2

The Light Lightness models allow remote control and configuration of dimmable lights on a mesh device.

The Light Lightness models can represent light in the following ways:

- *Actual*: Lightness is represented on a perceptually uniform lightness scale.
- *Linear*: Lightness is represented on a linear scale.

The relationship between the *Actual* and the *Linear* representations is the following:

*Light (Linear) = (Light (Actual))*:sup:`2`

Bindings with other states are always made to the *Actual* representation.

The Light Lightness models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

The application can select whether to use the Actual or Linear representation.
To do so, use the following options in the API at compile time:

* :kconfig:option:`CONFIG_BT_MESH_LIGHTNESS_ACTUAL` - Used by default.
* :kconfig:option:`CONFIG_BT_MESH_LIGHTNESS_LINEAR`

Internally, the models always support both representations, so nodes with different representations can be used interchangeably.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   lightness_srv.rst
   lightness_cli.rst


Common types
************

This section lists the types common to the Light Lightness mesh models.

| Header file: :file:`include/bluetooth/mesh/lightness.h`

.. doxygengroup:: bt_mesh_lightness
   :project: nrf
   :members:
