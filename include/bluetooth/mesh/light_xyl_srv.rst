.. _bt_mesh_light_xyl_srv_readme:

Light xyL Server
################

.. contents::
   :local:
   :depth: 2

The Light xyL Server represents a single light on a mesh device.
This model is well suited for implementation in light sources with multi-color light emission.
It should be instantiated in the light fixture node.

Three states can be used to configure the lighting output of an element:

* Lightness - This state determines the lightness of a tunable white light emitted by an element.
* x - Determines the x coordinate on the CIE1931 color space chart of a color light emitted by an element.
* y - Determines the y coordinate on the CIE1931 color space chart of a color light emitted by an element.

The Light xyL Server adds two model instances in the composition data, in addition to the extended :ref:`bt_mesh_lightness_srv_readme` model.
These model instances share the states of the Light xyL Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Light xyL states, as the model instances can be bound to different application keys.
The following two model instances are added:

* Light xyL Server - Provides write access to the Lightness, x and y states for the user, in addition to read access to all meta states
* Light xyL Setup Server - Provides write access to Default xyL state and Range meta states, allowing configurator devices to set up a range for the x and y state, and a default xyL state

States
******

The xyL model contains the following states:

Lightness: ``uint16_t``
    The Lightness state represents the emitted light level of an element, and ranges from ``0`` to ``65535``.
    The Lightness state is shared by the extended :ref:`bt_mesh_lightness_srv_readme` model.

    The Lightness state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Lightness state is set to ``0`` on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Lightness state is set to Default Lightness on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Lightness state is set to the last known Light level (zero or non-zero).

    The Lightness state is held and managed by the extended :ref:`bt_mesh_lightness_srv_readme`.

x: ``uint16_t``
    The x state represents the x coordinate on the CIE1931 color space chart of a color light emitted by an element.
    This is a 16-bit unsigned integer representation of a scale from 0 to 1 using the formula:

    .. code-block:: console

       CIE1931_x = (Light xyL x) / 65535

    The x state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The x state is set to Default x on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The x state is set to Default x on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The x state is set to the last known x level.

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_xyl_srv_handlers` handler structure.

y: ``uint16_t``
    The y state represents the y coordinate on the CIE1931 color space chart of a color light emitted by an element.
    This is a 16-bit unsigned integer representation of a scale from 0 to 1 using the formula:

    .. code-block:: console

       CIE1931_y = (Light xyL y) / 65535

    The y state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The y state is set to Default y on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The y state is set to Default y on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The y state is set to the last known y level.

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_xyl_srv_handlers` handler structure.

Default xy: :c:struct:`bt_mesh_light_xy`
    The Default xy state is a meta state that controls the default x and y level.
    It is used when the light is turned on, but its exact state levels are not specified.

    The memory for the Default xy state is held by the model, and the application may receive updates on state changes through the
    :c:member:`bt_mesh_light_xyl_srv_handlers.default_update` callback.


Range: :c:struct:`bt_mesh_light_xyl_range`
    The Range state is a meta state that determines the accepted x and y level range.
    If the x or y level is set to a value outside the currently defined Range state value, it is moved to fit inside the range.
    If the Range state changes to exclude the current x or y level, the level should be changed accordingly.

    The memory for the Range state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_light_xyl_srv_handlers.range_update` callback.

Extended models
***************

The Light xyL Server extends the following model:

* :ref:`bt_mesh_lightness_srv_readme`

State of the extended Lightness Server model is partially controlled by the Light xyL Server, making it able to alter states like Lightness and the Default Lightness of the Lightness Server model.

Persistent storage
******************

The Light xyL Server stores the following information:

* Any changes to states Default xyL and Range
* The last known Lightness, x, and y levels

In addition, the model takes over the persistent storage responsibility of the :ref:`bt_mesh_lightness_srv_readme` model.

This information is used to reestablish the correct light configuration when the device powers up.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_xyl_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_xyl_srv.c`

.. doxygengroup:: bt_mesh_light_xyl_srv
   :project: nrf
   :members:
