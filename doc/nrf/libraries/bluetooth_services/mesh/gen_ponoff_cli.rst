.. _bt_mesh_ponoff_cli_readme:

Generic Power OnOff Client
##########################

.. contents::
   :local:
   :depth: 2

The Generic Power OnOff Client model remotely controls the state of a :ref:`bt_mesh_ponoff_srv_readme` model.

Unlike the Generic Power OnOff Server, the Generic Power OnOff Client only adds one model instance to the composition data.
The Generic Power OnOff Client may send messages to both the Generic Power OnOff Server and the Generic Power OnOff Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Generic Power OnOff Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_PONOFF_CLI`

mesh models ponoff instance get-all
	Print all instances of the Generic Power OnOff Client model on the device.


mesh models ponoff instance set <ElemIdx>
	Select the Generic Power OnOff Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models ponoff get
	Get the OnPowerUp state of the server.


mesh models ponoff set <PowUpState>
	Set the OnPowerUp state of the server and wait for a response.

	* ``PowUpState`` - OnPowerUp state value to set. Allowed values:
		* ``0`` - On power-up, sets the state to off.
		* ``1`` - On power-up, sets the state to on.
		* ``2`` - On power-up, restores the previous state value.


mesh models ponoff set-unack <PowUpState>
	Set the Generic Power OnOff state value without requesting a response.

	* ``PowUpState`` - OnPowerUp state value to set. Allowed values:
		* ``0`` - On power-up, sets the state to off.
		* ``1`` - On power-up, sets the state to on.
		* ``2`` - On power-up, restores the previous state value.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_ponoff_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_ponoff_cli.c`

.. doxygengroup:: bt_mesh_ponoff_cli
   :project: nrf
   :members:
