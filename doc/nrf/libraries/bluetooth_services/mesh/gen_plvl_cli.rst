.. _bt_mesh_plvl_cli_readme:

Generic Power Level Client
##########################

.. contents::
   :local:
   :depth: 2

The Generic Power Level Client model remotely controls the state of a Generic Power Level Server model.

Unlike the Generic Power Level Server model, the Generic Power Level Client only creates a single model instance in the mesh composition data.
The Generic Power Level Client may send messages to both the Generic Power Level Server and the Generic Power Level Setup Server, as long as it has the right application keys.

Extended models
****************

None.

Persistent storage
*******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Generic Power Level Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_PLVL_CLI`

mesh models plvl instance get-all
	Print all instances of the Generic Power Level Client model on the device.


mesh models plvl instance set <elem_idx>
	Select the Generic Power Level Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mesh models plvl get
	Get the Power Level of the bound server.


mesh models plvl set <lvl> [transition_time(ms) [delay(ms)]]
	Set the Power Level of the server and wait for a response.

	* ``lvl`` - Power Level value to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models plvl set-unack <lvl> [transition_time(ms) [delay(ms)]]
	Set the Power Level of the server without requesting a response.

	* ``lvl`` - Power Level value to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models plvl range-get
	Get the Power Range of the bound server.


mesh models plvl range-set <min> <max>
	Set the Power Range state value and wait for a response.

	* ``min`` - Minimum allowed Power Level.
	* ``max`` - Maximum allowed Power Level.


mesh models plvl range-set-unack <min> <max>
	Set the Generic Power Range state value without requesting a response.

	* ``min`` - Minimum allowed Power Level.
	* ``max`` - Maximum allowed Power Level.


mesh models plvl default-get
	Get the Default Power state of the bound server.


mesh models plvl default-set <lvl>
	Set the Default Power state of the server and wait for a response.

	* ``lvl`` - Default Power value to set.


mesh models plvl default-set-unack <lvl>
	Set the Default Power state of the server without requesting a response.

	* ``lvl`` - Default Power value to set.


mesh models plvl last-get
	Get the last non-zero Power Level of the bound server.


API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_plvl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_plvl_cli.c`

.. doxygengroup:: bt_mesh_plvl_cli
   :project: nrf
   :members:
