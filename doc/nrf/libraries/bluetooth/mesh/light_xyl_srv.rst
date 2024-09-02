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

These model instances share the states of the Light xyL Server, but accept different messages.
This allows for a fine-grained control of the access rights for the Light xyL states, as the model instances can be bound to different application keys.
The following two model instances are added:

* Light xyL Server - Provides write access to the Lightness, x and y states for the user, in addition to read access to all meta states
* Light xyL Setup Server - Provides write access to Default xyL state and Range meta states, allowing configurator devices to set up a range for the x and y state, and a default xyL state

The extended :ref:`bt_mesh_lightness_srv_readme` model should be defined and initialized separately, and the pointer to this model should be passed to the Light xyL Server initialization macro.

Model composition
*****************

The extended Light Lightness Server shall be instantiated on the same element:

+------------------------+
| Element N              |
+========================+
| Light Lightness Server |
+------------------------+
| Light xyL Server       |
+------------------------+

The Light xyL Server structure does not contain a Light Lightness Server instance, so this must be instantiated separately.

In the application code, this would look like this:

.. code-block:: c

   static struct bt_mesh_lightness_srv lightess_srv = BT_MESH_LIGHTNESS_SRV_INIT(&lightness_cb);

   static struct bt_mesh_light_xyl_srv xyl_srv =
 	BT_MESH_LIGHT_XYL_SRV_INIT(&lightness_srv, &xyl_handlers);

   static struct bt_mesh_elem elements[] = {
    	BT_MESH_ELEM(
    		1, BT_MESH_MODEL_LIST(
    		   BT_MESH_MODEL_LIGHTNESS_SRV(&lightness_srv),
    		   BT_MESH_MODEL_LIGHT_XYL_SRV(&xyl_srv)),
    		BT_MESH_MODEL_NONE),
   };


.. _bt_mesh_light_xyl_hsl_srv:

Usage with the Light HSL Server
===============================

Just like the Light xyL Server, the :ref:`bt_mesh_light_hsl_srv_readme` provides the ability to control colored lights.
In some cases, it may be desirable to use the Light HSL and Light xyL Servers to provide two different interfaces for controlling the same light.
To achieve this, the Light xyL and Light HSL Server models should be instantiated on the same element, and share the same Light Lightness Server.

With the Light HSL Server's corresponding :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` models, the composition data should look like this:

.. table::
   :align: center

   =======================  =================  =======================
   Element N                Element N+1        Element N+2
   =======================  =================  =======================
   Light Lightness Server   Light Hue Server   Light Saturation Server
   Light HSL Server
   Light xyL Server
   =======================  =================  =======================


In the application code, this would look like this:

.. code-block:: c

   static struct bt_mesh_lightness_srv lightess_srv =
      BT_MESH_LIGHTNESS_SRV_INIT(&lightness_cb);

   static struct bt_mesh_light_hsl_srv hsl_srv =
   	BT_MESH_LIGHT_HSL_SRV_INIT(&lightness_srv, &hue_cb, &sat_cb);

   static struct bt_mesh_light_xyl_srv xyl_srv =
 	BT_MESH_LIGHT_XYL_SRV_INIT(&lightness_srv, &xyl_handlers);

   static struct bt_mesh_elem elements[] = {
   	BT_MESH_ELEM(
   		1, BT_MESH_MODEL_LIST(
            BT_MESH_MODEL_LIGHTNESS_SRV(&lightness_srv),
            BT_MESH_MODEL_LIGHT_HSL_SRV(&hsl_srv),
            BT_MESH_MODEL_LIGHT_XYL_SRV(&xyl_srv)),
   		BT_MESH_MODEL_NONE),
   	BT_MESH_ELEM(
   		2, BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_HUE_SRV(&hsl_srv.hue)),
   		BT_MESH_MODEL_NONE),
   	BT_MESH_ELEM(
   		3, BT_MESH_MODEL_LIST(BT_MESH_MODEL_LIGHT_SAT_SRV(&hsl_srv.sat)),
   		BT_MESH_MODEL_NONE),
   };

While there is just one shared instance of the Light Lightness Server controlling the Lightness in this configuration, the light's hue and saturation level may be controlled independently by the Light xyL and Light HSL Servers.
The binding between the Light xyL Server's x and y states, and the Light HSL Server's Hue and Saturation states is application-specific.

Even though there are no qualification tests verifying the binding between the color spectrum states of the Light xyL and Light HSL Servers, application developers are strongly encouraged to implement the general characteristics of bound states:

* Any changes to one state should be immediately reflected in the other.
* If publication is enabled for any of the models, a change to the bound state of one of the models should be published on both.
* Any range limitations on one of the bound states should be respected when setting the value of the other bound state.

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

State of the extended Light Lightness Server model is partially controlled by the Light xyL Server, making it able to alter states like Lightness and Default Lightness of the Light Lightness Server model.

Persistent storage
******************

The Light xyL Server stores the following information:

* Any changes to states Default xyL and Range
* The last known Lightness, x, and y levels

In addition, the model takes over the persistent storage responsibility of the :ref:`bt_mesh_lightness_srv_readme` model.

This information is used to reestablish the correct light configuration when the device powers up.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Light xyL Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

The Light xyL Server can use the :ref:`emergency data storage (EMDS) <emds_readme>` together with persistent storage to:

* Extend the flash memory life expectancy.
* Reduce the use of resources by reducing the number of writes to flash memory.

If option :kconfig:option:`CONFIG_EMDS` is enabled, the Light xyL Server continues to store the default xyL and range states to the flash memory through the settings library, but the last known Lightness, x, and y levels are stored by using the :ref:`EMDS <emds_readme>` library. The values stored by :ref:`EMDS <emds_readme>` will be lost at first boot when the :kconfig:option:`CONFIG_EMDS` is enabled.
This split is done so the values that may change often are stored on shutdown only, while the rarely changed values are immediately stored in flash memory.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_xyl_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/light_xyl_srv.c`

.. doxygengroup:: bt_mesh_light_xyl_srv
   :project: nrf
   :members:
