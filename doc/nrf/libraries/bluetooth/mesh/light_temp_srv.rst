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

The Light CTL Temperature Server stores the following information:

* Any changes to the Default Light Temperature/Delta UV and Temperature Range states.
* The last known Temperature level.

This information is used to reestablish the correct Temperature level when the device powers up.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Light CTL Temperature Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

The Light CTL Temperature Server can use the :ref:`emergency data storage (EMDS) <emds_readme>` together with persistent storage to:

* Extend the flash memory life expectancy.
* Reduce the use of resources by reducing the number of writes to flash memory.

If option :kconfig:option:`CONFIG_EMDS` is enabled, the Light CTL Temperature Server continues to store the default light temperature and temperature range states to the flash memory through the settings library, but the last known color temperature level is stored by using the :ref:`EMDS <emds_readme>` library. The values stored by :ref:`EMDS <emds_readme>` will be lost at first boot when the :kconfig:option:`CONFIG_EMDS` is enabled.
This split is done so the values that may change often are stored on shutdown only, while the rarely changed values are immediately stored in flash memory.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/light_temp_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_temp_srv.c`

.. doxygengroup:: bt_mesh_light_temp_srv
   :project: nrf
   :members:
