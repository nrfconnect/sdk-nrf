.. _bt_mesh_loc_cli_readme:

Generic Location Client
#######################

.. contents::
   :local:
   :depth: 2

The Generic Location Client model can get and set the state of a :ref:`bt_mesh_loc_srv_readme` model remotely.

Unlike the Generic Location Server model, the Generic Location Client only creates a single model instance in the mesh composition data.
The Generic Location Client may send messages to both the Generic Location Server and the Generic Location Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Generic Location Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LOC_CLI`

mesh models loc instance get-all
	Print all instances of the Generic Location Client model on the device.


mesh models loc instance set <elem_idx>
	Select the Generic Location Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mesh models loc global-get
	Get the current global location value of the Generic Location state.


mesh models loc global-set <latitude> <longitude> <altitude>
	Set the global location value of the Generic Location state and wait for a response.

	* ``latitude`` - Global WGS84 North coordinate in degrees.
	* ``longitude`` - Global WGS84 East coordinate in degrees.
	* ``altitude`` - Global altitude above the WGS84 datum in meters.


mesh models loc global-set-unack <latitude> <longitude> <altitude>
	Set the global location value of the Generic Location state without requesting a response.

	* ``latitude`` - Global WGS84 North coordinate in degrees.
	* ``longitude`` - Global WGS84 East coordinate in degrees.
	* ``altitude`` - Global altitude above the WGS84 datum in meters.


mesh models loc local-get
	Get the current local location value of the Generic Location state.


mesh models loc local-set <north> <east> <altitude> <floor> [time_delta(ms) [precision_mm [is_mobile]]]
	Set the local location value of the Generic Location state and wait for a response.

	* ``north`` - Local north position in decimeters.
	* ``east`` - Local east position in decimeters.
	* ``altitude`` - Local altitude in decimeters.
	* ``floor`` - Floor number.
	* ``time_delta`` - If present, defines the time since the previous position update in milliseconds.
	* ``precision_mm`` - If present, defines the precision of the location in millimeters.
	* ``is_mobile`` - If present, defines whether the device is movable.


mesh models loc local-set-unack <north> <east> <altitude> <floor> [time_delta(ms) [precision_mm [is_mobile]]]
	Set the local location value of the Generic Location state without requesting a response.

	* ``north`` - Local north position in decimeters.
	* ``east`` - Local east position in decimeters.
	* ``altitude`` - Local altitude in decimeters.
	* ``floor`` - Floor number.
	* ``time_delta`` - If present, defines the time since the previous position update in milliseconds.
	* ``precision_mm`` - If present, defines the precision of the location in millimeters.
	* ``is_mobile`` - If present, defines whether the device is movable.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_loc_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_loc_cli.c`

.. doxygengroup:: bt_mesh_loc_cli
   :project: nrf
   :members:
