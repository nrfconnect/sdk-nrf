.. _bt_mesh_light_ctl_cli_readme:

Light CTL Client
################

.. contents::
   :local:
   :depth: 2

The Color-Tunable Light (CTL) Client model remotely controls the state of the :ref:`bt_mesh_light_ctl_srv_readme` model, and the :ref:`bt_mesh_light_temp_srv_readme` model.

Unlike the server models, the Light CTL Client only creates a single model instance in the mesh composition data.
The Light CTL Client can send messages to the Light CTL Server, the Light CTL Setup Server and the Light CTL Temperature Server, as long as it has the right application keys.

.. note::
   By specification, the Light CTL Temperature Server must be located in another element than the Light CTL Server models.
   The user must ensure that the publish parameters of the client instance point to the correct element address when using the function calls in the client API.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Light CTL Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_LIGHT_CTL_CLI`

mdl_ctl instance get-all
	Print all instances of the Light CTL Client model on the device.


mdl_ctl instance set <elem_idx>
	Select the Light CTL Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mdl_ctl get
	Get the current Light CTL state value.


mdl_ctl set <light> <temp> <delta> [transition_time_ms [delay_ms]]
	Set the Light CTL state value and wait for a response.

	* ``light`` - Light level to set.
	* ``temp`` - Light temperature value to set.
	* ``delta`` - Delta UV value to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_ctl set-unack <light> <temp> <delta> [transition_time_ms [delay_ms]]
	Set the Light CTL state value without waiting for a response.

	* ``light`` - Light level value to set.
	* ``temp`` - Light temperature value to set.
	* ``delta`` - Delta UV value to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_ctl temp-get
	Get the current Light CTL Temperature state value.


mdl_ctl temp-set <light> <temp> <delta> [transition_time_ms [delay_ms]]
	Set the Light CTL Temperature state value and wait for a response.

	* ``temp`` - Light temperature value to set.
	* ``delta`` - Delta UV value to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_ctl temp-set-unack <light> <temp> <delta> [transition_time_ms [delay_ms]]
	Set the Light CTL Temperature state value.

	* ``temp`` - Light temperature value to set.
	* ``delta`` - Delta UV value to set.
	* ``transition_time_ms`` - If present, defines the transition time in the message in milliseconds.
	* ``delay_ms`` - If present, defines the delay in the message in milliseconds.


mdl_ctl default-get
	Get the current Light CTL Default state value.


mdl_ctl default-set <light> <temp> <delta>
	Set the Light CTL Default state value and wait for a response.

	* ``light`` - Light level value to set.
	* ``temp`` - Light temperature value to set.
	* ``delta`` - Delta UV value to set.


mdl_ctl default-set-unack <light> <temp> <delta>
	Set the default CTL value of the server without requesting a response.

	* ``light`` - Light level value to set.
	* ``temp`` - Light temperature value to set.
	* ``delta`` - Delta UV value to set.


mdl_ctl temp-range-get
	Get the current Light CTL Temperature Range state value.


mdl_ctl temp-range-set <min> <max>
	Set the Light CTL Temperature Range state value and wait for a response.

	* ``min`` - Minimum allowed light temperature value.
	* ``max`` - Maximum allowed light temperature value.


mdl_ctl temp-range-set-unack <min> <max>
	Set the Light CTL Temperature Range state value.

	* ``min`` - Minimum allowed light temperature value.
	* ``max`` - Maximum allowed light temperature value.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctl_cli.c`

.. doxygengroup:: bt_mesh_light_ctl_cli
   :project: nrf
   :members:
