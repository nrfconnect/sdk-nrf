.. _bt_mesh_light_xyl_readme:

Light xyL models
################

.. contents::
   :local:
   :depth: 2

The xyL models represent a light source based on a modified CIE xyY color space, one of the CIE1931 color spaces.
The CIE1931 color spaces are the first defined quantitative links between distributions of wavelengths in the electromagnetic visible spectrum, and physiologically perceived colors in human color vision.
The CIE1931 color space chart is utilized by the models to determine the color light emitted by an element.
The xyL models allow remote control and configuration of a light device with x and y chromaticity coordinates support in a mesh network.

.. figure:: images/CIExy1931.png
   :scale: 50 %
   :alt: CIE1931 color space chromaticity diagram

The Light xyL models can represent the lightness states on a linear or perceptually uniform lightness scale.
See :ref:`bt_mesh_lightness_readme` for details.

The Light xyL models also feature their own common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   light_xyl_cli.rst
   light_xyl_srv.rst


Common types
************

This section lists the types common to the Light xyL mesh models.

| Header file: :file:`include/bluetooth/mesh/light_xyl.h`

.. doxygengroup:: bt_mesh_light_xyl
   :project: nrf
   :members:
