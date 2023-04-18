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


mesh models loc instance set <ElemIdx>
	Select the Generic Location Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models loc global-get
	Get the current global location value of the Generic Location state.


mesh models loc global-set <Lat> <Long> <Alt>
	Set the global location value of the Generic Location state and wait for a response.

	* ``Lat`` - Global WGS84 North coordinate in degrees.
	* ``Long`` - Global WGS84 East coordinate in degrees.
	* ``Alt`` - Global altitude above the WGS84 datum in meters.


mesh models loc global-set-unack <Lat> <Long> <Alt>
	Set the global location value of the Generic Location state without requesting a response.

	* ``Lat`` - Global WGS84 North coordinate in degrees.
	* ``Long`` - Global WGS84 East coordinate in degrees.
	* ``Alt`` - Global altitude above the WGS84 datum in meters.


mesh models loc local-get
	Get the current local location value of the Generic Location state.


mesh models loc local-set <North> <East> <Alt> <Floor> [TimeDlt(ms) [Prec(mm) [IsMobile]]]
	Set the local location value of the Generic Location state and wait for a response.

	* ``North`` - Local north position in decimeters.
	* ``East`` - Local east position in decimeters.
	* ``Alt`` - Local altitude in decimeters.
	* ``Floor`` - Floor number.
	* ``TimeDlt`` - If present, defines the time since the previous position update in milliseconds.
	* ``Prec`` - If present, defines the precision of the location in millimeters.
	* ``IsMobile`` - If present, defines whether the device is movable.


mesh models loc local-set-unack <North> <East> <Alt> <Floor> [TimeDlt(ms) [Prec(mm) [IsMobile]]]
	Set the local location value of the Generic Location state without requesting a response.

	* ``North`` - Local north position in decimeters.
	* ``East`` - Local east position in decimeters.
	* ``Alt`` - Local altitude in decimeters.
	* ``Floor`` - Floor number.
	* ``TimeDlt`` - If present, defines the time since the previous position update in milliseconds.
	* ``Prec`` - If present, defines the precision of the location in millimeters.
	* ``IsMobile`` - If present, defines whether the device is movable.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_loc_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_loc_cli.c`

.. doxygengroup:: bt_mesh_loc_cli
   :project: nrf
   :members:
