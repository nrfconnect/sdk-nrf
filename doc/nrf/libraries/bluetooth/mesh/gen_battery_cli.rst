.. _bt_mesh_battery_cli_readme:

Generic Battery Client
######################

.. contents::
   :local:
   :depth: 2

The Generic Battery Client model can query the state of a :ref:`bt_mesh_battery_srv_readme` model remotely, but does not have any ability to control the Battery state.

Extended models
****************

None.

Persistent storage
*******************

None.

Shell commands
**************

The Bluetooth Mesh shell subsystem provides a set of commands to interact with the Generic Battery Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_BATTERY_CLI`

mesh models battery instance get-all
	Print all instances of the Generic Battery Client model on the device.


mesh models battery instance set <ElemIdx>
	Select the Generic Battery Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models battery get
	Get the battery status of the bound server.


API documentation
******************

| Header file: :file:`include/bluetooth/mesh/gen_battery_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_battery_cli.c`

.. doxygengroup:: bt_mesh_battery_cli
   :project: nrf
   :members:
