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

mesh models lightness instance get-all
	Print all instances of the Light Lightness Client model on the device.


mesh models lightness instance set <ElemIdx>
	Select the Light Lightness Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models lightness get
	Get the Light Level value of the bound server.


mesh models lightness set <Actual>|<Linear> [TransTime(ms) [Delay(ms)]]
	Set the Light Level value and wait for a response.

	* ``Actual|Linear`` - The Light Level value to be set. By default, the *Actual* representation of the value is used. If :kconfig:option:`CONFIG_BT_MESH_LIGHTNESS_LINEAR` is enabled, the *Linear* representation of the value is used.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models lightness set-unack <Actual>|<Linear> [TransTime(ms) [Delay(ms)]]
	Set the Light Level value without requesting a response.

	* ``Actual|Linear`` - The Light Level value to be set. By default, the *Actual* representation of the value is used. If :kconfig:option:`CONFIG_BT_MESH_LIGHTNESS_LINEAR` is enabled, the *Linear* representation of the value is used.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models lightness range-get
	Get the Light Range state value of the bound server.


mesh models lightness range-set <Min> <Max>
	Set the Light Range state value and wait for a response.

	* ``Min`` - Minimum allowed Light Level value.
	* ``Max`` - Maximum allowed Light Level value.


mesh models lightness range-set-unack <Min> <Max>
	Set the Light Range state value without requesting a response.

	* ``Min`` - Minimum allowed Light Level value.
	* ``Max`` - Maximum allowed Light Level value.


mesh models lightness default-get
	Get the Default Light state value of the bound server.


mesh models lightness default-set <Lvl>
	Set the Default Light state value of the server and wait for a response.

	* ``Lvl`` - The Default Light state value to be set.


mesh models lightness default-set-unack <Lvl>
	Set the Default Light state value of the server without requesting a response.

	* ``Lvl`` - The Default Light state value to be set.


mesh models lightness last-get
	Get the last non-zero Light Level value of the bound server.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/lightness_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/lightness_cli.c`

.. doxygengroup:: bt_mesh_lightness_cli
   :project: nrf
   :members:
