.. _bt_mesh_lightness_readme:

Light Lightness models
######################

The Light Lightness models allow remote control and configuration of dimmable lights on a mesh device.

The Light Lightness models can represent light in the following ways:

- *Actual*: Lightness is represented on a perceptually uniform lightness scale.
- *Linear*: Lightness is represented on a linear scale.

The application can select whether to use the Actual or Linear representation.
To do so, use :option:`CONFIG_BT_MESH_LIGHTNESS_ACTUAL` or :option:`CONFIG_BT_MESH_LIGHTNESS_LINEAR` configuration options, respectively, in the API at compile time.
By default, the Actual representation is used.
Internally, the models will always support both representations, so nodes with different representations can be be used interchangeably.

.. note::
    The relationship between the *Actual* and the *Linear* representations is the following:

    *Light (Linear) = (Light (Actual))*:sup:`2`

    Bindings with other states are always made to the *Actual* representation.

The following Light Lightness models are available:

.. toctree::
   :maxdepth: 1
   :glob:

   lightness_srv.rst
   lightness_cli.rst

Common types
=============

| Header file: :file:`include/bluetooth/mesh/lightness.h`

.. doxygengroup:: bt_mesh_lightness
   :project: nrf
   :members:
