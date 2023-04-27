.. _bt_mesh_lightness_srv_readme:

Light Lightness Server
######################

.. contents::
   :local:
   :depth: 2

The Light Lightness Server represents a single light on a mesh device.
It should be instantiated in the light fixture node.

The Light Lightness Server adds the following new model instances in the composition data, in addition to the extended :ref:`bt_mesh_lvl_srv_readme` and :ref:`bt_mesh_ponoff_srv_readme` models:

* Light Lightness Server
* Light Lightness Setup Server

These model instances share the states of Light Lightness Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Light Lightness states, as these model instances can be bound to different application keys:

* The Light Lightness Server only provides write access to the Light state for the user, in addition to read access to all the meta states.
* The Light Lightness Setup Server provides write access to the Default Light and Light Range meta states.
  This allows the configurator devices to set up the range and the default value for the Light state.

States
======

The Generic Power OnOff Server model contains the following states:

Light: ``uint16_t``
    The Light state represents the emitted light level of an element, and ranges from ``0`` to ``65535``.
    The Light state is bound to the Generic Level State of the extended :ref:`bt_mesh_lvl_srv_readme`:

    .. code-block:: console

       Light (Actual) = Generic Level + 32768

    The Generic OnOff state of :ref:`bt_mesh_onoff_srv_readme` (extended through the :ref:`bt_mesh_ponoff_srv_readme`) is derived from the Light state:

    .. code-block:: console

       Generic OnOff = (Light > 0)

    Conversely, if the Generic OnOff state is changed to ``Off``, the Light state is set to ``0``.
    If the Generic OnOff state is changed to ``On`` and the Default Light state is set, the Light state is set to the value of the Default Light state.
    If the Generic OnOff state is changed to ``On`` and the Default Light state is not set, the Light state is set to the last known non-zero value.

    The Light state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Light state is set to ``0`` on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Light state is set to Default Light on power-up, or to the last known non-zero Light state if the Default Light is not set.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Light state is set to the last known Light level (zero or non-zero).

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_lightness_srv_handlers` handler structure.

    .. note::
        If the Lightness Server is part of an xyL, CTL or HSL Server, it will publish the xyL, CTL or HSL status whenever the Light state changes.
        This is not handled automatically by the xyL, CTL or HSL Servers.

Default Light: ``int16_t``
    The Default Light state is a meta state that controls the default non-zero Light level.
    It is used when the light is turned on, but its exact level is not specified.

    The memory for the Default Light state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_lightness_srv_handlers.default_update` callback.

    The Default Light state uses the configured lightness representation.

Light Range: :c:struct:`bt_mesh_lightness_range`
    The Light Range state is a meta state that determines the accepted Light level range.

    If the Light level is set to a value outside the current Light Range, it is moved to fit inside the range.

    If the Light Range changes to exclude the current Light level, the Light level should be changed accordingly.

    .. note::
        The Light level may always be set to zero, even if this is outside the current Light Range.

    The memory for the Light Range state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_lightness_srv_handlers.range_update` callback.

    The Light Range state uses the configured lightness representation.

Extended models
================

The Light Lightness Server extends the following models:

* :ref:`bt_mesh_lvl_srv_readme`
* :ref:`bt_mesh_ponoff_srv_readme`

As the states of both extended models are bound to states in the Light Lightness Server, the states of the extended models are not exposed directly to the application.

Persistent storage
===================

The Light Lightness Server stores the following information:

* Any changes to the Default Light and Light Range states.
* The last known non-zero Light level.
* Whether the light is on or off.

This information is used to reestablish the correct Light level when the device powers up.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Light Lightness Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

The Light Lightness Server can use the :ref:`emergency data storage (EMDS) <emds_readme>` together with persistent storage for the following purposes:

* Extend the flash memory life expectancy.
* Reduce the use of resources by reducing the number of writes to flash memory.

If option :kconfig:option:`CONFIG_EMDS` is enabled, the Light Lightness Server continues to store the default light and light range states to the flash memory through the settings library, but the last known non-zero Light level and whether the light is on or off are stored by using the :ref:`EMDS <emds_readme>` library. The values stored by :ref:`EMDS <emds_readme>` will be lost at first boot when the :kconfig:option:`CONFIG_EMDS` is enabled.
This split is done so the values that may change often are stored on shutdown only, while the rarely changed values are immediately stored in flash memory.

API documentation
==================

| Header file: :file:`include/bluetooth/mesh/lightness_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/lightness_srv.c`

.. doxygengroup:: bt_mesh_lightness_srv
   :project: nrf
   :members:
