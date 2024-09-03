.. _bt_mesh_light_ctrl_cli_readme:

Light Lightness Control Client
##############################

.. contents::
   :local:
   :depth: 2

.. note::
   This model interacts with the new sensor API introduced as of |NCS| v2.6.0.
   As a consequence, parts of the model API have been changed as well.
   The old API is deprecated, but still available by enabling the Kconfig option :kconfig:option:`CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE`.
   The Kconfig option is enabled by default in the deprecation period.
   See the documentation for |NCS| versions prior to v2.6.0 for documentation about the old sensor API.

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

The Bluetooth Mesh shell subsystem provides a set of commands to interact with the Light LC Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LIGHT_CTRL_CLI`

mesh models ctrl instance get-all
	Print all instances of the Light LC Client model on the device.


mesh models ctrl instance set <ElemIdx>
	Select the Light LC Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models ctrl mode-get
	Get the Light Lightness Control Server's current Mode.


mesh models ctrl mode-set <Enable(off, on)>
	Set the Light Lightness Control Server's current Mode and wait for a response.

	* ``Enable`` - Mode to set.


mesh models ctrl mode-set-unack <Enable(off, on)>
	Set the Light Lightness Control Server's current Mode without requesting a response.

	* ``Enable`` - Mode to set.


mesh models ctrl occupancy-get
	Get the Light Lightness Control Server's current Occupancy Mode.


mesh models ctrl occupancy-set <Enable(off, on)>
	Set the Light Lightness Control Server's current Occupancy Mode and wait for a response.

	* ``Enable`` - Occupancy Mode to set.


mesh models ctrl occupancy-set-unack <Enable(off, on)>
	Set the Light Lightness Control Server's current Occupancy Mode without requesting a response.

	* ``Enable`` - Occupancy Mode to set.


mesh models ctrl light-onoff-get
	Get the Light Lightness Control Server's current OnOff state.


mesh models ctrl light-onoff-set <OnOff> [TransTime(ms) [Delay(ms)]]
	Tell the Light Lightness Control Server to turn the light on or off, and wait for a response.

	* ``OnOff`` - OnOff state value to set. Use *on*, *enable*, or any non-zero value to enable the state.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctrl light-onoff-set-unack <OnOff> [TransTime(ms) [Delay(ms)]]
	Tell the Light Lightness Control Server to turn the light on or off without requesting a response.

	* ``OnOff`` - OnOff state value to set. Use *on*, *enable*, or any non-zero value to enable the state.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctrl prop-get <ID>
	Get a property value of the Light Lightness Control Server.

	* ``ID`` - ID of the property to get. See :c:enum:`bt_mesh_light_ctrl_prop` for a list of supported properties.


mesh models ctrl prop-set <ID> <Val>
	Set a property value of the Light Lightness Control Server and wait for a response.

	* ``ID`` - ID of the property to set. See :c:enum:`bt_mesh_light_ctrl_prop` for a list of supported properties.
	* ``Val`` - The new value of the property.


mesh models ctrl prop-set-unack <ID> <Val>
	Set a property value of the Light Lightness Control Server without requesting a response.

	* ``ID`` - ID of the property to set. See :c:enum:`bt_mesh_light_ctrl_prop` for a list of supported properties.
	* ``Val`` - The new value of the property.


mesh models ctrl coeff-get <ID>
	Get a Regulator Coefficient value of the Light Lightness Control Server.

	* ``ID`` - ID of the coefficient to get. See :c:enum:`bt_mesh_light_ctrl_coeff` for a list of supported coefficients.


mesh models ctrl coeff-set <ID> <Val>
	Set a Regulator Coefficient value of the Light Lightness Control Server and wait for a response.

	* ``ID`` - ID of the coefficient to set. See :c:enum:`bt_mesh_light_ctrl_coeff` for a list of supported coefficients.
	* ``Val`` - New coefficient value.


mesh models ctrl coeff-set-unack <ID> <Val>
	Set a Regulator Coefficient value of the Light Lightness Control Server without requesting a response.

	* ``ID`` - ID of the coefficient to set. See :c:enum:`bt_mesh_light_ctrl_coeff` for a list of supported coefficients.
	* ``Val`` - New coefficient value.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctrl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctrl_cli.c`

.. doxygengroup:: bt_mesh_light_ctrl_cli
   :project: nrf
   :members:
