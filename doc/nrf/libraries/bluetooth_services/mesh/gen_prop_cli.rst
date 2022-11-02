.. _bt_mesh_prop_cli_readme:

Generic Property Client
#######################

.. contents::
   :local:
   :depth: 2

The Generic Property Client model can access properties from a :ref:`Property Server <bt_mesh_prop_srv_readme>` remotely.
The Property Client can talk directly to all types of Property Servers, but only if it shares an application key with the target server.

Generally, the Property Client should only target User Property Servers, unless it is part of a network administrator node that is responsible for configuring the other mesh nodes.

To ease the configuration, the Property Client can be paired with a Client Property Server that lists which properties this Property Client will request.

Extended models
***************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Generic Property Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_PROP_CLI`

mesh models prop instance get-all
	Print all instances of the Generic Property Client model on the device.


mesh models prop instance set <elem_idx>
	Select the Generic Property Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mesh models prop cli-props-get <id>
	Get the list of Generic Client Properties of the bound server.

	* ``id`` - A starting Client Property ID present within an element.


mesh models prop props-get <kind>
	Get the list of properties of the bound server.

	* ``kind`` - Kind of Property Server to query. Allowed values:
		* ``0`` - Manufacturer Property Server.
		* ``1`` - Admin Property Server.
		* ``2`` - User Property Server.


mesh models prop prop-get <kind> <id>
	Get the value of a property of a server.

	* ``kind`` - Kind of Property Server to query. Allowed values:
		* ``0`` - Manufacturer Property Server.
		* ``1`` - Admin Property Server.
		* ``2`` - User Property Server.
		* ``3`` - Client Property Server.
	* ``id`` - ID of the property.


mesh models prop user-prop-set <id> <hex_str_val>
	Set a property value of the User Property Server and wait for response.

	* ``id`` - Property ID.
	* ``hex_str_val`` - Property value.


mesh models prop user-prop-set-unack <id> <hex_str_val>
	Set a property value of the User Property Server without requesting a response.

	* ``id`` - Property ID.
	* ``hex_str_val`` - Property value.


mesh models prop admin-prop-set <id> <access> <hex_str_val>
	Set a property value of the Admin Property Server and wait for response.

	* ``id`` - Property ID.
	* ``access`` - User access flags for the property. Allowed values:
		* ``0`` - Access to the property is prohibited.
		* ``1`` - Property may be read.
		* ``2`` - Property may be written.
		* ``3`` - Property may be read or written.
	* ``hex_str_val`` - Property value.


mesh models prop admin-prop-set-unack <id> <access> <hex_str_val>
	Set a property value of the Admin Property Server without requesting a response.

	* ``id`` - Property ID.
	* ``access`` - User access flags for the property. Allowed values:
		* ``0`` - Access to the property is prohibited.
		* ``1`` - Property may be read.
		* ``2`` - Property may be written.
		* ``3`` - Property may be read or written.
	* ``hex_str_val`` - Property value.


mesh models prop mfr-prop-set <id> <access>
	Set the user access of a property of the Manufacturer Property Server and wait for response.

	* ``id`` - Property ID.
	* ``access`` - User access flags for the property. Allowed values:
		* ``0`` - Access to the property is prohibited.
		* ``1`` - Property may be read.
		* ``2`` - Property may be written.
		* ``3`` - Property may be read or written.


mesh models prop mfr-prop-set-unack <id> <access>
	Set the user access of a property of the Manufacturer Property Server without requesting a response.

	* ``id`` - Property ID.
	* ``access`` - User access flags for the property. Allowed values:
		* ``0`` - Access to the property is prohibited.
		* ``1`` - Property may be read.
		* ``2`` - Property may be written.
		* ``3`` - Property may be read or written.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/gen_prop_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_prop_cli.c`

.. doxygengroup:: bt_mesh_prop_cli
   :project: nrf
   :members:
