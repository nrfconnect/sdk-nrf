.. _bt_mesh_light_temp_srv_readme:

Light CTL Temperature Server
############################

.. contents::
   :local:
   :depth: 2

The Color-Tunable Light (CTL) Temperature Server represents a single light on a mesh device.
It should be instantiated in the light fixture node.

The Light CTL Temperature Server is normally instantiated as part of the :ref:`bt_mesh_light_ctl_srv_readme` model, but may be instantiated as a stand-alone model.

Two states can be used to configure the lighting output of an element:

* Temperature - This state determines the color temperature of tunable white light emitted by an element.
* Delta UV - This state determines the distance from the black body curve.
  The color temperatures fall on the black body locus.

The Light CTL Temperature Server adds the following new model instances in the composition data, in addition to the extended :ref:`bt_mesh_lvl_srv_readme` model:

* Light CTL Temperature Server

States
======

Temperature: ``uint16_t``
    The Temperature state represents the color temperature of the tunable white light emitted by an element, and ranges from ``800`` to ``20000``.

    The Temperature state is bound to the Generic Level State of the extended :ref:`bt_mesh_lvl_srv_readme`:

    .. parsed-literal::
       :class: highlight

       Light CTL Temperature = *T_MIN* + (Generic Level + 32768) * (*T_MAX* - *T_MIN*) / 65535

    In the above formula, *T_MIN* and *T_MAX* are values representing the Light CTL Temperature Range Min and Light CTL Temperature Range Max states.

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_ctl_srv_handlers` handler structure.

Delta UV: ``int16_t``
    The Temperature state represents the distance from the black body curve.
    The color temperatures all fall on the black body locus (curve).
    This is a 16-bit signed integer representation of a -1 to +1 scale using the formula:

    .. code-block:: console

       Represented Delta UV = (Light CTL Delta UV) / 32768

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_ctl_srv_handlers` handler structure.

Temperature Range: :c:struct:`bt_mesh_light_temp_range`
    The Temperature Range state is a meta state that determines the accepted Temperature level range.

    If the Temperature level is set to a value outside the current Temperature Range, it is moved to fit inside the range.

    Access to the Temperature Range state can only be done when the Light CTL Temperature Server is associated with an instance of the :ref:`bt_mesh_light_ctl_srv_readme` model.
    The default value for the Temperature Range is  ``800`` for T_MIN, and ``20000`` for T_MAX.

Extended models
================

The Light CTL Temperature Server extends the following model:

* :ref:`bt_mesh_lvl_srv_readme`

The states of the extended model are bound to the states in the Light CTL Temperature Server, and not directly exposed to the application.

Persistent storage
===================

None.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/light_temp_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_temp_srv.c`

.. doxygengroup:: bt_mesh_light_temp_srv
   :project: nrf
   :members:
