.. _bt_mesh_dtt_cli_readme:

Generic Default Transition Time Client
######################################

.. contents::
   :local:
   :depth: 2

The Generic DTT Client remotely controls the transition time state of a :ref:`bt_mesh_dtt_srv_readme`.

The client can be used to set up the server's default transition time behavior when changing states in other models on the same element.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Generic DTT Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_DTT_CLI`

mesh models dtt instance get-all
	Print all instances of the Generic Default Transition Time Client model on the device.


mesh models dtt instance set <elem_idx>
	Select the Generic Default Transition Time Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mesh models dtt get
	Get the Default Transition Time of the bound server.


mesh models dtt set <transition_time(ms)>
	Set the Default Transition Time of the server and wait for a response.

	* ``transition_time`` - Default transition time in milliseconds.


mesh models dtt set-unack <transition_time(ms)>
	Set the Default Transition Time of the server without requesting a response.

	* ``transition_time`` - Default transition time in milliseconds.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_dtt_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_dtt_cli.c`

.. doxygengroup:: bt_mesh_dtt_cli
   :project: nrf
   :members:
