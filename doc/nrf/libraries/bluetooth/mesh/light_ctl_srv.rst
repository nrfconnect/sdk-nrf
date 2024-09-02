.. _bt_mesh_light_ctl_srv_readme:

Light CTL Server
################

.. contents::
   :local:
   :depth: 2

The Color-Tunable Light (CTL) Server represents a single light on a mesh device.
It should be instantiated in the light fixture node.

Three states can be used to configure the lighting output of an element:

* Lightness - This state determines the lightness of a tunable white light emitted by an element.
* Temperature - This state determines the color temperature of tunable white light emitted by an element.
* Delta UV - This state determines the distance from the black body curve.
  The color temperatures fall on the black body locus.

The Light CTL Server adds two model instances in the composition data, in addition to the extended :ref:`bt_mesh_lightness_srv_readme` model.
These model instances share the states of the Light CTL Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Light CTL states, as the model instances can be bound to different application keys.
The following two model instances are added:

* Light CTL Server - Provides write access to Lightness, Temperature and Delta UV states for the user, in addition to read access to all meta states
* Light CTL Setup Server - Provides write access to Default CTL state and Temperature Range meta states, allowing configurator devices to set up a temperature range and a default CTL state

In addition to the extended Light Lightness Server model, the Light CTL Server also requires a :ref:`bt_mesh_light_temp_srv_readme` to be instantiated on a subsequent element.
The Light Temperature Server should reference the :c:member:`bt_mesh_light_ctl_srv.temp_srv`.

Conventionally, the Light Temperature Server model is instantiated on the very next element, and the composition data looks as presented below.

.. code-block:: c

    static struct bt_mesh_light_ctl_srv light_ctl_srv = BT_MESH_LIGHT_CTL_SRV_INIT(&lightness_handlers, &light_temp_handlers);

    static struct bt_mesh_elem elements[] = {
        BT_MESH_ELEM(
            1,
            BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_CTL_SRV(&light_ctl_srv)),
            BT_MESH_MODEL_NONE),
        BT_MESH_ELEM(
            2,
            BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_TEMP_SRV(&light_ctl_srv.temp_srv)),
            BT_MESH_MODEL_NONE),
    };

The Light CTL Server does not require any message handler callbacks, as all its states are bound to the included :ref:`bt_mesh_lightness_srv_readme` and :ref:`bt_mesh_light_temp_srv_readme` models.
The Lightness and Light Temperature Server callbacks will pass pointers to :c:member:`bt_mesh_light_ctl_srv.lightness_srv` and :c:member:`bt_mesh_light_ctl_srv.temp_srv`, respectively.

.. note::

    The Light CTL Server will verify that its internal Light Temperature Server is instantiated on a subsequent element on startup.
    If the Light Temperature Server is missing or instantiated on the same or a preceding element, the Bluetooth® Mesh startup procedure will fail, and the device will not be responsive.

States
======

The Generic Power OnOff Server model (extended by the Light Lightness Server model) contains the following states:

Lightness: ``uint16_t``
    The Lightness state represents the emitted light level of an element, and ranges from ``0`` to ``65535``.
    The Lightness state is shared by the extended :ref:`bt_mesh_lightness_srv_readme` model.


    The Lightness state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Lightness state is set to ``0`` on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Lightness state is set to Default Lightness on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Lightness state is set to the last known Light level (zero or non-zero).

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_ctl_srv_handlers` handler structure.

Temperature: ``uint16_t``
    The Temperature state represents the color temperature of the tunable white light emitted by an element.
    It ranges from ``800`` to ``20000``, and is shared by the associated :ref:`bt_mesh_light_temp_srv_readme`.

    The Temperature state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Temperature state is set to Default Temperature on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Temperature state is set to Default Temperature on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Temperature state is set to the last known Temperature level.

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_ctl_srv_handlers` handler structure.

Delta UV: ``int16_t``
    The Temperature state represents the distance from the black body curve.
    The color temperatures all fall on the black body locus (curve).
    This is a 16-bit signed integer representation of a -1 to +1 scale using the following formula:

    .. code-block:: console

       Represented Delta UV = (Light CTL Delta UV) / 32768

    The Delta UV state of the Light CTL Server is shared by the associated :ref:`bt_mesh_light_temp_srv_readme`, and its power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Delta UV state is set to Default Delta UV on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Delta UV state is set to Default Delta UV on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Delta UV state is set to the last known Delta UV level.

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_ctl_srv_handlers` handler structure.

Default CTL: :c:struct:`bt_mesh_light_ctl`
    The Default CTL state is a meta state that controls the default Lightness, Temperature and Delta UV level.
    It is used when the light is turned on, but its exact state levels are not specified.

    The memory for the Default Light state is held by the model, and the application may receive updates on state changes through the
    :c:member:`bt_mesh_lightness_srv_handlers.default_update` callback.

    The Default Light state uses the configured lightness representation.

Temperature Range: :c:struct:`bt_mesh_light_temp_range`
    The Temperature Range state is a meta state that determines the accepted Temperature level range.
    If the Temperature level is set to a value outside the current Temperature Range, it is moved to fit inside the range.
    If the Temperature Range changes to exclude the current Temperature level, the Temperature level should be changed accordingly.

    The Temperature Range state of the Light CTL Server is shared by the associated :ref:`bt_mesh_light_temp_srv_readme`.

    The memory for the Temperature Range state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_light_ctl_srv_handlers.temp_range_update` callback.


Extended models
================

The Light CTL Server extends the following model:

* :ref:`bt_mesh_lightness_srv_readme`

The state of the extended Light Lightness Server model is for the most part bound to states in the Light CTL Server.
The only exception is the Lightness range state, which is exposed to the application through the :c:member:`bt_mesh_light_ctl_srv_handlers.lightness_range_update` callback of the Light CTL Server model.

In addition to the extended Light Lightness Server model, the Light CTL Server model is associated with a Light Temperature model on a subsequent element.
Unlike the extended models, the associated models do not share subscription lists, but still share states.

Persistent storage
===================

The Light CTL Server stores the following information:

* Any changes to the Default CTL and Temperature Range states
* The last known Lightness, Temperature and Delta UV level

This information is used to reestablish the correct light configuration when the device powers up.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/light_ctl_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctl_srv.c`

.. doxygengroup:: bt_mesh_light_ctl_srv
   :project: nrf
   :members:
