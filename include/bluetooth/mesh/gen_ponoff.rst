.. _bt_mesh_ponoff_readme:

Generic Power OnOff models
##########################

The Generic Power OnOff models allow remote control of the power up behavior
on a mesh device.

There are two Generic Power OnOff models:

- :ref:`bt_mesh_ponoff_srv_readme`
- :ref:`bt_mesh_ponoff_cli_readme`

.. _bt_mesh_ponoff_srv_readme:

Generic Power OnOff Server
==========================

The Generic Power OnOff server controls the power up behavior of other models
on its element. The Generic Power OnOff server depends on the
:option:`CONFIG_BT_SETTINGS` to be enabled to work properly. Unless
:option:`CONFIG_BT_SETTINGS` is explicitly disabled, including the Generic
Power OnOff Server will enable :option:`CONFIG_BT_SETTINGS`.

Generic Power OnOff Server adds two model instances in the composition data:

- The Generic Power OnOff Server
- The Generic Power OnOff Setup Server

The two model instances share the states of the Generic Power OnOff Server,
but accept different messages. This allows fine-grained control of the access
rights for the Generic Power OnOff state, as the two model instances can be
bound to different application keys.

The Generic Power OnOff Server only provides read access to the Generic
On Power Up state.

The Generic Power OnOff Setup Server provides write access to the Generic
On Power Up state, allowing configurator devices to change the power up
behavior of the element.

States
*******

**Generic On Power Up**: :cpp:type:`bt_mesh_on_power_up`.

The Generic On Power Up state controls the initial value of the extended
Generic OnOff state when the device powers up:

- On Power Up is
  :cpp:enumerator:`BT_MESH_ON_POWER_UP_OFF <bt_mesh_ponoff::BT_MESH_ON_POWER_UP_OFF>`:
  The OnOff state is initialized to Off.
- On Power Up is
  :cpp:enumerator:`BT_MESH_ON_POWER_UP_ON <bt_mesh_ponoff::BT_MESH_ON_POWER_UP_ON>`:
  The OnOff state is initialized to On. If any other states are bound to the On
  Power Up state, they are initialized to their default values.
- On Power Up is
  :cpp:enumerator:`BT_MESH_ON_POWER_UP_RESTORE <bt_mesh_ponoff::BT_MESH_ON_POWER_UP_RESTORE>`:
  The OnOff state is initialized to its last known value. If any other states
  are bound to the On Power Up state, they are initialized to their default
  values.

The memory for the Generic On Power Up state is contained in the model
structure, and state changes can be observed with the
:cpp:member:`bt_mesh_ponoff_srv::update` callback.

Extended models
****************

The Generic Power OnOff Server extends the following models:

- :ref:`bt_mesh_onoff_srv_readme`
- :ref:`bt_mesh_dtt_srv_readme`

The On Power Up state is bound to the Generic OnOff state of the extended
Generic OnOff model through its power up behavior. No other state bindings
are present, and the callbacks for both the Generic OnOff server and the
Generic DTT server are forwarded to the application as they are.

Persistent storage
*******************

The Generic On Power Up state is stored persistently, along with the current
Generic OnOff state of the extended :ref:`bt_mesh_onoff_srv_readme`.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_ponoff_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_ponoff_srv.c`

.. doxygengroup:: bt_mesh_ponoff_srv
   :project: nrf
   :members:

----

.. _bt_mesh_ponoff_cli_readme:

Generic Power OnOff Client
==========================

The Generic Power OnOff Client model remotely controls the state of a Generic
Power OnOff Server model.

Contrary to the Generic Power OnOff Server, the Generic Power OnOff Client only
adds one model instance to the composition data. The Generic Power OnOff Client
may send messages to both the Generic Power OnOff Server and the Generic Power
OnOff Setup server, as long as it has the right application keys.

Extended models
****************

None.

Persistent storage
*******************

None.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_ponoff_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_ponoff_cli.c`

.. doxygengroup:: bt_mesh_ponoff_cli
   :project: nrf
   :members:

----

Common types
=============

| Header file: :file:`include/bluetooth/mesh/gen_ponoff.h`

.. doxygengroup:: bt_mesh_ponoff
   :project: nrf
   :members:
