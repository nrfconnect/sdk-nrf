.. _bt_mesh_light_hsl_readme:

Light HSL models
################

.. contents::
   :local:
   :depth: 2

The Hue Saturation Lightness (HSL) models allow remote control of colored lights.
While the :ref:`CTL <bt_mesh_light_ctl_readme>` and xyL models operate in color spaces designed specifically for illumination, the HSL color space is most commonly used in computer imagery and television broadcasts.
While easy to visualize and explain to end users, the HSL does not map well to common illumination properties such as color temperature, and does not take variation in the human perception of color into account.

The HSL models are best suited for applications where the color of the light is a primary function, such as decorative lighting.
For applications where illumination is the primary function of the light, and its color and temperature is secondary, the CTL and xyL models might be better alternatives.
However, neither the Bluetooth® Mesh model specification nor the |NCS| put any restrictions on the application areas for the different models.
Developers may freely use the model best suited for their application.

On the light fixture side, the HSL Server models are separated into three independent models:

* :ref:`bt_mesh_light_hue_srv_readme`
* :ref:`bt_mesh_light_sat_srv_readme`
* :ref:`bt_mesh_lightness_srv_readme`

Each of these models can be instantiated independently, or be combined using the :ref:`bt_mesh_light_hsl_srv_readme`.
See the documentation of each individual model for details.

The Light HSL models have a set of common types, listed below.
For types common to all models, see :ref:`bt_mesh_models`.

Conversion from HSL to RGB
**************************

The theoretical conversion from HSL to RGB is well-defined, and the :c:func:`bt_mesh_light_hsl_to_rgb` function provides an easy conversion between the two color spaces, for use with RGB LED hardware.
While the conversion between HSL and RGB is universal, the properties of the light emitting hardware will influence the perceived linearity of transitions in the color space.
For instance, although changing the hue of the light should only affect its color, the perceived lightness and saturation may also fluctuate if the light emitting hardware has a non-linear response, as perceived by the human eye.
These fluctuations cannot be modeled efficiently in the model code, and must be accounted for and calibrated in the driver.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   light_hsl_cli.rst
   light_hsl_srv.rst
   light_hue_srv.rst
   light_sat_srv.rst


Common API
**********

This section lists the API common to the Light HSL mesh models.

| Header file: :file:`include/bluetooth/mesh/light_hsl.h`

.. doxygengroup:: bt_mesh_light_hsl
   :project: nrf
   :members:
