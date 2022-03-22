.. _bt_mesh_light_xyl_cli_readme:

Light xyL Client
################

.. contents::
   :local:
   :depth: 2

The xyl Client model remotely controls the state of the :ref:`bt_mesh_light_xyl_srv_readme` model.

Unlike the server model, the client only creates a single model instance in the mesh composition data.
The Light xyL Client can send messages to the Light xyL Server and the Light xyL Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Light xyL Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LIGHT_XYL_CLI`

mdl_xyl instance get-all
	Print all instances of the Light xyL Client model on the device.


mdl_xyl instance set <elem_idx>
	Select the Light xyL Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mdl_xyl get
	Get the Light xyL state value of the bound server.


mdl_xyl set <light> <x> <y> [transition_time_ms [delay_ms]]
	Set the Light xyL state value and wait for a response.

	* ``light`` - Lightness level to set.
	* ``x`` - x level to set.
	* ``y`` - y level to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_xyl set-unack <light> <x> <y> [transition_time_ms [delay_ms]]
	Set the Light xyL state value without requesting a response.

	* ``light`` - Lightness level to set.
	* ``x`` - x level to set.
	* ``y`` - y level to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_xyl target-get
	Get the Light xyL target state value of the bound server.


mdl_xyl default-get
	Get the default xyL value of the bound server.


mdl_xyl default-set <light> <x> <y>
	Set the default xyL value of the server and wait for a response.

	* ``light`` - Default lightness level to be set.
	* ``x`` - Default x level to be set.
	* ``y`` - Default y level to be set.


mdl_xyl default-set-unack <light> <x> <y>
	Set the Default xyL state value of the server without requesting a response.

	* ``light`` - Default lightness level to be set.
	* ``x`` - Default x level to be set.
	* ``y`` - Default y level to be set.


mdl_xyl range-get
	Get the Light xyL Range state value of the bound server.


mdl_xyl range-set <x_min> <y_min> <x_max> <y_max>
	Set the Light xyL Range state value and wait for a response.

	* ``x_min`` - Minimum allowed x value.
	* ``y_min`` - Minimum allowed y value.
	* ``x_max`` - Maximum allowed x value.
	* ``y_max`` - Maximum allowed y value.


mdl_xyl range-set-unack <x_min> <y_min> <x_max> <y_max>
	Set the Light xyL Range state value without requesting a response.

	* ``x_min`` - Minimum allowed x value.
	* ``y_min`` - Minimum allowed y value.
	* ``x_max`` - Maximum allowed x value.
	* ``y_max`` - Maximum allowed y value.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_xyl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_xyl_cli.c`

.. doxygengroup:: bt_mesh_light_xyl_cli
   :project: nrf
   :members:
