.. _bt_mesh_light_hue_srv_readme:

Light Hue Server
################

.. contents::
   :local:
   :depth: 2

The Light Hue Server controls the hue of the light color.
It should be instantiated in the light fixture node.

The Light Hue Server adds a single model instance to the composition data, in addition to the extended :ref:`bt_mesh_lvl_srv_readme`.
Although the Light Hue Server contains ``range`` and ``default`` states like other lighting models, the states cannot be accessed directly by other nodes without a :ref:`bt_mesh_light_hsl_srv_readme`.

The Light Hue Server model's Hue state has certain rules for the wrap-around behavior that need to be considered when writing the application.
Unlike the other Lightness models, the Hue state's change structure :c:struct:`bt_mesh_light_hue` has an additional field :c:member:`bt_mesh_light_hue.direction`.
This field tells whether the current Hue state should increase or decrease in order to reach the target value, when the Hue state transition is not instantaneous.

When the Hue state is not limited by the Hue Range state (the minimum value of the hue range is ``0`` and the maximum value is ``65535``), the Hue state can wrap around.
This, for example, can happen when the target value is lower than the current one, but the Hue state should increment as claimed by the :c:member:`bt_mesh_light_hue.direction` field.
This is because the :c:member:`bt_mesh_light_hue.direction` field points to the shortest path towards the target value.

The Hue state also wraps around when the minimum value of the Hue state is greater than the maximum value.
In this case, the state wraps around when the value reaches the end of the type range.
This happens because the :c:member:`bt_mesh_light_hue.direction` field points to the direction of the allowed range of the Hue state values.

When the application gets a call to the :c:member:`bt_mesh_light_hue_srv_handlers.set` callback, the transition always stops at the target value represented by the :c:member:`bt_mesh_light_hue.lvl` field.
But, if the application gets a call to the :c:member:`bt_mesh_light_hue_srv_handlers.move_set` callback, it must start a continuous transition of the Hue state in the direction and rate determined by the :c:struct:`bt_mesh_light_hue_move` structure.
This move transition continues until the Light Hue Server receives a new ``set`` or ``move_set`` message, or the device reboots.

States
******

Hue: ``uint16_t``
    The Hue controls the light's angle on the color wheel.
    The Hue values ``0`` - ``65535`` can represent any position on the color spectrum, starting at red, moving through green and blue, before coming back to red at the maximum value.
    The Hue state should wrap around when it reaches the end of the spectrum, so that a *Generic Level Move* message to the extended Generic Level Server should start a never-ending cycling through the hue spectrum, unless it is restricted by the Hue Range.

    The Hue state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Hue state is set to the Default Hue state on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Hue state is set to the Default Hue state on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Hue state is set to the last known Hue value on power-up.

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_hue_srv_handlers` handler structure.

    .. note::
        If the Light Hue Server is part of a Light HSL Server, the application must publish the HSL status using :c:func:`bt_mesh_light_hsl_srv_pub` whenever the Hue state changes.
        This is not handled automatically by the Light HSL Server.

Hue Range: :c:struct:`bt_mesh_light_hsl_range`
    The Hue Range is a meta state that defines the valid range for the Hue state.
    The Hue should never go outside the Hue Range.
    The maximum value can be less than the minimum value.

    The memory for the Hue Range state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_light_hue_handlers.range_update` callback.

Default Hue: ``uint16_t``
    The Default Hue state is only used when the model is instantiated as part of a :ref:`bt_mesh_light_hsl_srv_readme`.
    The Default Hue determines the initial Hue when the node is powered on, and the On Power Up state of the :ref:`bt_mesh_light_hsl_srv_readme`'s extended :ref:`bt_mesh_ponoff_srv_readme` is ``ON`` or ``OFF``.

    The memory for the Default Hue state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_light_hue_handlers.default_update` callback.

Extended models
****************

The Light Hue Server extends the following models:

* :ref:`bt_mesh_lvl_srv_readme`

As the state of the extended model is bound to the Hue state, the extended model is not exposed directly to the application.

Persistent storage
*******************

The Light Hue Server stores the following information:

* Any changes to the Default Hue and Hue Range states.
* The last known Hue level.

This information is used to reestablish the correct Hue level when the device powers up.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Light Hue Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

The Light Hue Server can use the :ref:`emergency data storage (EMDS) <emds_readme>` together with persistent storage to:

* Extend the flash memory life expectancy.
* Reduce the use of resources by reducing the number of writes to flash memory.

If option :kconfig:option:`CONFIG_EMDS` is enabled, the Light Hue Server continues to store the default Hue and Hue range states to the flash memory through the settings library, but the last known Hue level is stored by using the :ref:`EMDS <emds_readme>` library. The values stored by :ref:`EMDS <emds_readme>` will be lost at first boot when the :kconfig:option:`CONFIG_EMDS` is enabled.
This split is done so the values that may change often are stored on shutdown only, while the rarely changed values are immediately stored in flash memory.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/light_hue_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_hue_srv.c`

.. doxygengroup:: bt_mesh_light_hue_srv
   :project: nrf
   :members:
