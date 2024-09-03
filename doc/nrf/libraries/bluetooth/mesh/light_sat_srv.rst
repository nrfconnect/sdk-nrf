.. _bt_mesh_light_sat_srv_readme:

Light Saturation Server
#######################

.. contents::
   :local:
   :depth: 2

The Light Saturation Server controls the saturation of the light color.
It should be instantiated in the light fixture node.

The Light Saturation Server adds a single model instance to the composition data, in addition to the extended :ref:`bt_mesh_lvl_srv_readme`.
Although the Light Saturation Server contains ``range`` and ``default`` states like other lighting models, the states cannot be accessed directly by other nodes without a :ref:`bt_mesh_light_hsl_srv_readme`.

States
******

Saturation: ``uint16_t``
    The Saturation controls the light's color saturation.
    If the Saturation of a light is ``0``, the light's color is a completely gray scale, that is, white light.
    If the Saturation of a light is the maximum value of ``65535``, the color should be fully saturated.

    The Saturation state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

    * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Saturation state is set to the Default Saturation state on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Saturation state is set to the Default Saturation state on power-up.
    * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Saturation state is set to the last known Saturation value on power-up.

    Your application is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_light_sat_srv_handlers` handler structure.

    .. note::
        If the Saturation Server is part of an HSL Server, the application must publish the HSL status using :c:func:`bt_mesh_light_hsl_srv_pub` whenever the Saturation state changes.
        This is not handled automatically by the HSL Server.

Saturation Range: :c:struct:`bt_mesh_light_hsl_range`
    The Saturation Range is a meta state that defines the valid range for the Saturation state.
    The Saturation should never go outside the Saturation Range.

    The memory for the Saturation Range state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_light_sat_handlers.range_update` callback.

Default Saturation: ``uint16_t``
    The Default Saturation state is only used when the model is instantiated as part of a :ref:`bt_mesh_light_hsl_srv_readme`.
    The Default Saturation determines the initial Saturation when the node is powered on, and the On Power Up state of the :ref:`bt_mesh_light_hsl_srv_readme`'s extended :ref:`bt_mesh_ponoff_srv_readme` is ``ON`` or ``OFF``.

    The memory for the Default Saturation state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_light_sat_handlers.default_update` callback.

Extended models
****************

The Light Saturation Server extends the following models:

* :ref:`bt_mesh_lvl_srv_readme`

As the state of the extended model is bound to the Saturation state, the extended model is not exposed directly to the application.

Persistent storage
*******************

The Light Saturation Server stores the following information:

* Any changes to the Default Saturation and Saturation Range states.
* The last known Saturation level.

This information is used to reestablish the correct Saturation level when the device powers up.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Light Saturation Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

The Light Saturation Server can use the :ref:`emergency data storage (EMDS) <emds_readme>` together with persistent storage for the following purposes:

* Extend the flash memory life expectancy.
* Reduce the use of resources by reducing the number of writes to flash memory.

If option :kconfig:option:`CONFIG_EMDS` is enabled, the Light Saturation Server continues to store the default saturation and saturation range states to the flash memory through the settings library, but the last known saturation level is stored by using the :ref:`EMDS <emds_readme>` library.
This split is done so the values that may change often are stored on shutdown only, while the rarely changed values are immediately stored in flash memory.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/light_sat_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_sat_srv.c`

.. doxygengroup:: bt_mesh_light_sat_srv
   :project: nrf
   :members:
