.. _bt_mesh_light_hsl_cli_readme:

Light HSL Client
################

.. contents::
   :local:
   :depth: 2

The Light HSL Client model remotely controls the state of the :ref:`bt_mesh_light_hsl_srv_readme`, :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` models.

Unlike the Light HSL Server model, the Client only creates a single model instance in the mesh composition data.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Light HSL Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LIGHT_HSL_CLI`

mesh models hsl instance get-all
	Print all instances of the Light HSL Client model on the device.


mesh models hsl instance set <elem_idx>
	Select the Light HSL Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mesh models hsl get
	Get the Light HSL state value of the bound server.


mesh models hsl set <light> <hue> <saturation> [transition_time(ms) [delay(ms)]]
	Set the Light HSL state value and wait for a response.

	* ``light`` - Lightness level to set.
	* ``hue`` - Hue level to set.
	* ``saturation`` - Saturation level to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl set-unack <light> <hue> <saturation> [transition_time(ms) [delay(ms)]]
	Set the Light HSL state value without requesting for a response.

	* ``light`` - Lightness level to set.
	* ``hue`` - Hue level to set.
	* ``saturation`` - Saturation level to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl target-get
	Get the Light HSL target state value of the bound server.


mesh models hsl default-get
	Get the default HSL value of the bound server.


mesh models hsl default-set <light> <hue> <saturation>
	Set the default HSL value of the server and wait for a response.

	* ``light`` - Default lightness level to set.
	* ``hue`` - Default hue level to set.
	* ``saturation`` - Default saturation level to set.


mesh models hsl default-set-unack <light> <hue> <saturation>
	Set the Default Light state value of the server without requesting a response.

	* ``light`` - Default lightness level to set.
	* ``hue`` - Default hue level to set.
	* ``saturation`` - Default saturation level to set.


mesh models hsl range-get
	Get the Light HSL Range state value of the bound server.


mesh models hsl range-set <hue_min> <hue_max> <sat_min> <sat_max>
	Set the Light HSL Range state value and wait for a response.

	* ``hue_min`` - Minimum allowed hue value.
	* ``hue_max`` - Maximum allowed hue value.
	* ``sat_min`` - Minimum allowed saturation value.
	* ``sat_max`` - Maximum allowed saturation value.


mesh models hsl range-set-unack <hue_min> <hue_max> <sat_min> <sat_max>
	Set the Light HSL Range state value without requesting a response.

	* ``hue_min`` - Minimum allowed hue value.
	* ``hue_max`` - Maximum allowed hue value.
	* ``sat_min`` - Minimum allowed saturation value.
	* ``sat_max`` - Maximum allowed saturation value.


mesh models hsl hue-get
	Get the Light Hue state value of the bound server.


mesh models hsl hue-set <lvl> [transition_time(ms) [delay(ms)]]
	Set the Light Hue state value and wait for a response.

	* ``lvl`` - Hue level to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl hue-set-unack <lvl> [transition_time(ms) [delay(ms)]]
	Set the Light Hue state value without requesting a response.

	* ``lvl`` - Hue level to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl saturation-get
	Get the Light Saturation state value of the bound server.


mesh models hsl saturation-set <lvl> [transition_time(ms) [delay(ms)]]
	Set the Light Saturation state value and wait for a response.

	* ``lvl`` - Saturation level to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl saturation-set-unack <lvl> [transition_time(ms) [delay(ms)]]
	Set the Light Saturation state value without requesting a response.

	* ``lvl`` - Saturation level to set.
	* ``transition_time`` - If present, defines the transition time in the message in milliseconds.
	* ``delay`` - If present, defines the delay in the message in milliseconds.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_hsl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_hsl_cli.c`

.. doxygengroup:: bt_mesh_light_hsl_cli
   :project: nrf
   :members:
