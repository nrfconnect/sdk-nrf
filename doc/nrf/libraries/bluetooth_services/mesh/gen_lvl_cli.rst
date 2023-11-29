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

The Bluetooth Mesh shell subsystem provides a set of commands to interact with the Generic Level Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LVL_CLI`

mesh models lvl instance get-all
	Print all instances of the Generic Level Client model on the device.


mesh models lvl instance set <ElemIdx>
	Select the Generic Level Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models lvl get
	Get the current Generic Level state value.


mesh models lvl set <Lvl> [TransTime(ms) [Delay(ms)]]
	Set the Generic Level state value and wait for a response.

	* ``Lvl`` - Level state value to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models lvl set-unack <Lvl> [TransTime(ms) [Delay(ms)]]
	Set the Generic Level state value without requesting a response.

	* ``Lvl`` - Level state value to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models lvl delta-set <Dlt> [TransTime(ms) [Delay(ms)]]
	Trigger a differential level state change for the server and wait for a response.

	* ``Dlt`` - Translation from the original value.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models lvl delta-set-unack <Dlt> [TransTime(ms) [Delay(ms)]]
	Trigger a differential level state change for the server without requesting a response.

	* ``Dlt`` - Translation from the original value.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models lvl move-set <Dlt> [TransTime(ms) [Delay(ms)]]
	Trigger a continuous level change for the server and wait for a response.

	* ``Dlt`` - Translation to make for every transition step.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models lvl move-set-unack <Dlt> [TransTime(ms) [Delay(ms)]]
	Trigger a continuous level change for the server without requesting a response.

	* ``Dlt`` - Translation to make for every transition step.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_lvl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_lvl_cli.c`

.. doxygengroup:: bt_mesh_lvl_cli
   :project: nrf
   :members:
