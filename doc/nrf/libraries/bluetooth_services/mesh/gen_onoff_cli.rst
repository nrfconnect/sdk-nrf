.. _bt_mesh_onoff_cli_readme:

Generic OnOff Client
####################

.. contents::
   :local:
   :depth: 2

The Generic OnOff Client model remotely controls the state of a :ref:`bt_mesh_onoff_srv_readme` model.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Generic OnOff Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_ONOFF_CLI`

mesh models onoff instance get-all
	Print all instances of the Generic OnOff Client model on the device.


mesh models onoff instance set <elem_idx>
	Select the Generic OnOff Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mesh models onoff get
	Get the OnOff state value of the bound server.


mesh models onoff set <onoff> [transition_time(ms) [delay(ms)]]
	Set the OnOff state value of the server and wait for a response.

	* ``onoff`` - OnOff state value to set. Use *on*, *enable*, or any non-zero value to enable the state.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models onoff set-unack <onoff> [transition_time(ms) [delay(ms)]]
	Set the OnOff state value of the server without requesting a response.

	* ``onoff`` - OnOff state value to set. Use *on*, *enable*, or any non-zero value to enable the state.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_onoff_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_onoff_cli.c`

.. doxygengroup:: bt_mesh_onoff_cli
   :project: nrf
   :members:
