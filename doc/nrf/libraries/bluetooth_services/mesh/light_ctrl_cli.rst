.. _bt_mesh_light_ctrl_cli_readme:

Light Lightness Control Client
##############################

.. contents::
   :local:
   :depth: 2

The Light Lightness Control (LC) Client configures and interacts with the :ref:`bt_mesh_light_ctrl_srv_readme`.

The Light LC Client creates a single model instance in the mesh composition data, and it can send messages to both the Light LC Server and the Light LC Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Light LC Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LIGHT_CTRL_CLI`

mesh models ctrl instance get-all
	Print all instances of the Light LC Client model on the device.


mesh models ctrl instance set <elem_idx>
	Select the Light LC Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mesh models ctrl mode-get
	Get the Light Lightness Control Server's current Mode.


mesh models ctrl mode-set <enable | disable>
	Set the Light Lightness Control Server's current Mode and wait for a response.

	* ``enable | disable`` - Mode to set.


mesh models ctrl mode-set-unack <enable | disable>
	Set the Light Lightness Control Server's current Mode without requesting a response.

	* ``enable | disable`` - Mode to set.


mesh models ctrl occupancy-get
	Get the Light Lightness Control Server's current Occupancy Mode.


mesh models ctrl occupancy-set <enable | disable>
	Set the Light Lightness Control Server's current Occupancy Mode and wait for a response.

	* ``enable | disable`` - Occupancy Mode to set.


mesh models ctrl occupancy-set-unack <enable | disable>
	Set the Light Lightness Control Server's current Occupancy Mode without requesting a response.

	* ``enable | disable`` - Occupancy Mode to set.


mesh models ctrl light-onoff-get
	Get the Light Lightness Control Server's current OnOff state.


mesh models ctrl light-onoff-set <onoff> [transition_time(ms) [delay(ms)]]
	Tell the Light Lightness Control Server to turn the light on or off, and wait for a response.

	* ``onoff`` - OnOff state value to set. Use *on*, *enable*, or any non-zero value to enable the state.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctrl light-onoff-set-unack <onoff> [transition_time(ms) [delay(ms)]]
	Tell the Light Lightness Control Server to turn the light on or off without requesting a response.

	* ``onoff`` - OnOff state value to set. Use *on*, *enable*, or any non-zero value to enable the state.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctrl prop-get <id>
	Get a property value of the Light Lightness Control Server.

	* ``id`` - ID of the property to get. See :c:enum:`bt_mesh_light_ctrl_prop` for a list of supported properties.


mesh models ctrl prop-set <id> <value>
	Set a property value of the Light Lightness Control Server and wait for a response.

	* ``id`` - ID of the property to set. See :c:enum:`bt_mesh_light_ctrl_prop` for a list of supported properties.
	* ``value`` - The new value of the property.


mesh models ctrl prop-set-unack <id> <value>
	Set a property value of the Light Lightness Control Server without requesting a response.

	* ``id`` - ID of the property to set. See :c:enum:`bt_mesh_light_ctrl_prop` for a list of supported properties.
	* ``value`` - The new value of the property.


mesh models ctrl coeff-get <id>
	Get a Regulator Coefficient value of the Light Lightness Control Server.

	* ``id`` - ID of the coefficient to get. See :c:enum:`bt_mesh_light_ctrl_coeff` for a list of supported coefficients.


mesh models ctrl coeff-set <id> <value>
	Set a Regulator Coefficient value of the Light Lightness Control Server and wait for a response.

	* ``id`` - ID of the coefficient to set. See :c:enum:`bt_mesh_light_ctrl_coeff` for a list of supported coefficients.
	* ``value`` - New coefficient value.


mesh models ctrl coeff-set-unack <id> <value>
	Set a Regulator Coefficient value of the Light Lightness Control Server without requesting a response.

	* ``id`` - ID of the coefficient to set. See :c:enum:`bt_mesh_light_ctrl_coeff` for a list of supported coefficients.
	* ``value`` - New coefficient value.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctrl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctrl_cli.c`

.. doxygengroup:: bt_mesh_light_ctrl_cli
   :project: nrf
   :members:
