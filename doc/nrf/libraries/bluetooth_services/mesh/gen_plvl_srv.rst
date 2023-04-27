.. _bt_mesh_plvl_srv_readme:

Generic Power Level Server
##########################

.. contents::
   :local:
   :depth: 2

The Generic Power Level Server controls the output power level of a peripheral on the mesh-enabled device.

Generic Power Level Server adds two model instances in the composition data:

* The Generic Power Level Server
* The Generic Power Level Setup Server

The two model instances share the states of the Generic Power Level Server, but accept different messages.
This allows fine-grained control of the access rights for the Generic Power Level states, as the two model instances can be bound to different application keys.

The Generic Power Level Server is the user-facing model instance in the pair, and only provides access to the Generic Power Level state, and its last non-zero value.

The Generic Power Level Setup Server provides access to the two metastates, Default Power and Power Range, allowing configurator devices to set up the range and default value for the Generic Power Level state.

States
******

The following states are available:

Generic Power Level: (``uint16_t``)
   The Generic Power Level state controls the power level of an element, and ranges from 0 to 65535.
   The Generic Power Level state is bound to the Generic Level State of the :ref:`bt_mesh_lvl_srv_readme`:

   .. code-block:: none

      Generic Power Level = Generic Level + 32768

..

   The Generic OnOff state of the :ref:`bt_mesh_onoff_srv_readme` (extended through the :ref:`bt_mesh_ponoff_srv_readme`) is derived from the Power state:

   .. code-block:: none

      Generic OnOff = Generic Power Level > 0

..

   Conversely, if the Generic OnOff state is changed to Off, the Generic Power Level is set to 0.
   If the Generic OnOff state is changed to On and the Default Level state is set, the Generic Power Level is set to the value of the Default Level state.
   If the Generic OnOff state is changed to On and the Default Level state is not set, the Generic Power Level state is set to the last known non-zero value.

   The Power state power-up behavior is determined by the On Power Up state of the extended :ref:`bt_mesh_ponoff_srv_readme`:

   * :c:enumerator:`BT_MESH_ON_POWER_UP_OFF` - The Power Level is set to 0 on power-up.
   * :c:enumerator:`BT_MESH_ON_POWER_UP_ON` - The Power Level is set to Default Level on power-up, or the last known non-zero Power Level if the Default Level is not set.
   * :c:enumerator:`BT_MESH_ON_POWER_UP_RESTORE` - The Power Level is set to the last known Power Level (zero or otherwise).

   The user is expected to hold the state memory and provide access to the state through the :c:struct:`bt_mesh_plvl_srv_handlers` handler structure.

Default Power: (``int16_t``)
   The Default Power state is a metastate that controls the default non-zero Generic Power Level.
   It is used when the Generic Power Level turns on, but its exact level is not specified.

   The memory for the Default Power state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_plvl_srv_handlers.default_update` callback.

Power Range: (:c:struct:`bt_mesh_plvl_range`)
   The Power Range state is a metastate that determines the accepted Generic Power Level range.

   If the Generic Power Level is set to a value outside the current Power Range, the actual Generic Power Level is moved to fit inside the range.

   If the Power Level Range changes to exclude the current Generic Power Level, the Generic Power Level should be changed accordingly.
   Note that the Generic Power Level may always be set to zero, even if this is outside the current Power Range.

   The memory for the Power Range state is held by the model, and the application may receive updates on state changes through the :c:member:`bt_mesh_plvl_srv_handlers.range_update` callback.

Extended models
***************

The Generic Power Level Server extends the following models:

* :ref:`bt_mesh_lvl_srv_readme`
* :ref:`bt_mesh_ponoff_srv_readme`

As the states of both extended models are bound to states in the Generic Power Level Server, the states of the extended models are not exposed directly to the application.

Persistent storage
******************

The Generic Power Level Server stores any changes to the Default Power and Power Range states, as well as the last known non-zero Generic Power Level and whether the Generic Power Level is on or off.
This information is used to reestablish the correct Generic Power Level when the device powers up.

If option :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Generic Power Level Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

The Generic Power Level Server can use the :ref:`emergency data storage (EMDS) <emds_readme>` together with persistent storage to:

* Extend the flash memory life expectancy.
* Reduce the use of resources by reducing the number of writes to flash memory.

If option :kconfig:option:`CONFIG_EMDS` is enabled, the Generic Power Level Server continues to store the default power and power range states to the flash memory through the settings library, but the last known level and whether the Generic Power Level is on or off are stored by using the :ref:`EMDS <emds_readme>` library. The values stored by :ref:`EMDS <emds_readme>` will be lost at first boot when the :kconfig:option:`CONFIG_EMDS` is enabled.
This split is done so the values that may change often are stored on shutdown only, while the rarely changed values are immediately stored in flash memory.

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_plvl_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_plvl_srv.c`

.. doxygengroup:: bt_mesh_plvl_srv
   :project: nrf
   :members:
