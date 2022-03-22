.. _bt_mesh_lightness_cli_readme:

Light Lightness Client
######################

.. contents::
   :local:
   :depth: 2

The Light Lightness Client model remotely controls the state of a :ref:`bt_mesh_lightness_srv_readme` model.

Unlike the Light Lightness Server model, the Light Lightness Client only creates a single model instance in the mesh composition data.
The Light Lightness Client can send messages to both the Light Lightness Server and the Light Lightness Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Light Lightness Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LIGHTNESS_CLI`

mdl_lightness instance get-all
	Print all instances of the Light Lightness Client model on the device.


mdl_lightness instance set <elem_idx>
	Select the Light Lightness Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mdl_lightness get
	Get the Light Level value of the bound server.


mdl_lightness set <lightness_actual>|<lightness_linear> [transition_time_ms [delay_ms]]
	Set the Light Level value and wait for a response.

	* ``lightness_actual|lightness_linear`` - The Light Level value to be set. By default the *Actual* representation of the value is used. If :kconfig:option:`CONFIG_BT_MESH_LIGHTNESS_LINEAR` is enabled, the *Linear* representation of the value is used.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_lightness set-unack <lightness_actual>|<lightness_linear> [transition_time_ms [delay_ms]]
	Set the Light Level value without requesting a response.

	* ``lightness_actual|lightness_linear`` - The Light Level value to be set. By default the *Actual* representation of the value is used. If :kconfig:option:`CONFIG_BT_MESH_LIGHTNESS_LINEAR` is enabled, the *Linear* representation of the value is used.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_lightness range-get
	Get the Light Range state value of the bound server.


mdl_lightness range-set <min> <max>
	Set the Light Range state value and wait for a response.

	* ``min`` - Minimum allowed Light Level value.
	* ``max`` - Maximum allowed Light Level value.


mdl_lightness range-set-unack <min> <max>
	Set the Light Range state value without requesting a response.

	* ``min`` - Minimum allowed Light Level value.
	* ``max`` - Maximum allowed Light Level value.


mdl_lightness default-get
	Get the Default Light state value of the bound server.


mdl_lightness default-set <lvl>
	Set the Default Light state value of the server and wait for a response.

	* ``lvl`` - The Default Light state value to be set.


mdl_lightness default-set-unack <lvl>
	Set the Default Light state value of the server without requesting a response.

	* ``lvl`` - The Default Light state value to be set.


mdl_lightness last-get
	Get the last non-zero Light Level value of the bound server.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/lightness_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/lightness_cli.c`

.. doxygengroup:: bt_mesh_lightness_cli
   :project: nrf
   :members:
