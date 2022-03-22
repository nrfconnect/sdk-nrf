.. _bt_mesh_lvl_cli_readme:

Generic Level Client
####################

.. contents::
   :local:
   :depth: 2

The Generic Level Client model remotely controls the state of a :ref:`bt_mesh_lvl_srv_readme` model.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Generic Level Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LVL_CLI`

mdl_lvl instance get-all
	Print all instances of the Generic Level Client model on the device.


mdl_lvl instance set <elem_idx>
	Select the Generic Level Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mdl_lvl get
	Get the current Generic Level state value.


mdl_lvl set <lvl> [transition_time_ms [delay_ms]]
	Set the Generic Level state value and wait for a response.

	* ``lvl`` - Level state value to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_lvl set-unack <lvl> [transition_time_ms [delay_ms]]
	Set the Generic Level state value without requesting a response.

	* ``level`` - Level state value to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_lvl delta-set <delta> [transition_time_ms [delay_ms]]
	Trigger a differential level state change for the server and wait for a response.

	* ``delta`` - Translation from the original value.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_lvl delta-set-unack <delta> [transition_time_ms [delay_ms]]
	Trigger a differential level state change for the server without requesting a response.

	* ``delta`` - Translation from the original value.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_lvl move-set <delta> [transition_time_ms [delay_ms]]
	Trigger a continuous level change for the server and wait for a response.

	* ``delta`` - Translation to make for every transition step.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_lvl move-set-unack <delta> [transition_time_ms [delay_ms]]
	Trigger a continuous level change for the server without requesting a response.

	* ``delta`` - Translation to make for every transition step.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_lvl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_lvl_cli.c`

.. doxygengroup:: bt_mesh_lvl_cli
   :project: nrf
   :members:
