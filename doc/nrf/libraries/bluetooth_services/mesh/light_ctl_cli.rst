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

mesh models ctl instance get-all
	Print all instances of the Light CTL Client model on the device.


mesh models ctl instance set <ElemIdx>
	Select the Light CTL Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models ctl get
	Get the current Light CTL state value.


mesh models ctl set <Light> <Temp> <Dlt> [TransTime(ms) [Delay(ms)]]
	Set the Light CTL state value and wait for a response.

	* ``Light`` - Light level to set.
	* ``Temp`` - Light temperature value to set.
	* ``Dlt`` - Delta UV value to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctl set-unack <Light> <Temp> <Dlt> [TransTime(ms) [Delay(ms)]]
	Set the Light CTL state value without waiting for a response.

	* ``Light`` - Light level value to set.
	* ``Temp`` - Light temperature value to set.
	* ``Dlt`` - Delta UV value to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctl temp-get
	Get the current Light CTL Temperature state value.


mesh models ctl temp-set <Light> <Temp> <Dlt> [TransTime(ms) [Delay(ms)]]
	Set the Light CTL Temperature state value and wait for a response.

	* ``Temp`` - Light temperature value to set.
	* ``Dlt`` - Delta UV value to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctl temp-set-unack <Light> <Temp> <Dlt> [TransTime(ms) [Delay(ms)]]
	Set the Light CTL Temperature state value.

	* ``Temp`` - Light temperature value to set.
	* ``Dlt`` - Delta UV value to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models ctl default-get
	Get the current Light CTL Default state value.


mesh models ctl default-set <Light> <Temp> <Dlt>
	Set the Light CTL Default state value and wait for a response.

	* ``Light`` - Light level value to set.
	* ``Temp`` - Light temperature value to set.
	* ``Dlt`` - Delta UV value to set.


mesh models ctl default-set-unack <Light> <Temp> <Dlt>
	Set the default CTL value of the server without requesting a response.

	* ``Light`` - Light level value to set.
	* ``Temp`` - Light temperature value to set.
	* ``Dlt`` - Delta UV value to set.


mesh models ctl temp-range-get
	Get the current Light CTL Temperature Range state value.


mesh models ctl temp-range-set <Min> <Max>
	Set the Light CTL Temperature Range state value and wait for a response.

	* ``Min`` - Minimum allowed light temperature value.
	* ``Max`` - Maximum allowed light temperature value.


mesh models ctl temp-range-set-unack <Min> <Max>
	Set the Light CTL Temperature Range state value.

	* ``Min`` - Minimum allowed light temperature value.
	* ``Max`` - Maximum allowed light temperature value.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctl_cli.c`

.. doxygengroup:: bt_mesh_light_ctl_cli
   :project: nrf
   :members:
