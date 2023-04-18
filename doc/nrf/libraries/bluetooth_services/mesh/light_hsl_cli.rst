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


mesh models hsl instance set <ElemIdx>
	Select the Light HSL Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models hsl get
	Get the Light HSL state value of the bound server.


mesh models hsl set <Light> <Hue> <Sat> [TransTime(ms) [Delay(ms)]]
	Set the Light HSL state value and wait for a response.

	* ``Light`` - Lightness level to set.
	* ``Hue`` - Hue level to set.
	* ``Sat`` - Saturation level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl set-unack <Light> <Hue> <Sat> [TransTime(ms) [Delay(ms)]]
	Set the Light HSL state value without requesting for a response.

	* ``Light`` - Lightness level to set.
	* ``Hue`` - Hue level to set.
	* ``Sat`` - Saturation level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl target-get
	Get the Light HSL target state value of the bound server.


mesh models hsl default-get
	Get the default HSL value of the bound server.


mesh models hsl default-set <Light> <Hue> <Sat>
	Set the default HSL value of the server and wait for a response.

	* ``Light`` - Default lightness level to set.
	* ``Hue`` - Default hue level to set.
	* ``Sat`` - Default saturation level to set.


mesh models hsl default-set-unack <Light> <Hue> <Sat>
	Set the Default Light state value of the server without requesting a response.

	* ``Light`` - Default lightness level to set.
	* ``Hue`` - Default hue level to set.
	* ``Sat`` - Default saturation level to set.


mesh models hsl range-get
	Get the Light HSL Range state value of the bound server.


mesh models hsl range-set <HueMin> <HueMax> <SatMin> <SatMax>
	Set the Light HSL Range state value and wait for a response.

	* ``HueMin`` - Minimum allowed hue value.
	* ``HueMax`` - Maximum allowed hue value.
	* ``SatMin`` - Minimum allowed saturation value.
	* ``SatMax`` - Maximum allowed saturation value.


mesh models hsl range-set-unack <HueMin> <HueMax> <SatMin> <SatMax>
	Set the Light HSL Range state value without requesting a response.

	* ``HueMin`` - Minimum allowed hue value.
	* ``HueMax`` - Maximum allowed hue value.
	* ``SatMin`` - Minimum allowed saturation value.
	* ``SatMax`` - Maximum allowed saturation value.


mesh models hsl hue-get
	Get the Light Hue state value of the bound server.


mesh models hsl hue-set <Lvl> [TransTime(ms) [Delay(ms)]]
	Set the Light Hue state value and wait for a response.

	* ``Lvl`` - Hue level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl hue-set-unack <Lvl> [TransTime(ms) [Delay(ms)]]
	Set the Light Hue state value without requesting a response.

	* ``Lvl`` - Hue level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl saturation-get
	Get the Light Saturation state value of the bound server.


mesh models hsl saturation-set <Lvl> [TransTime(ms) [Delay(ms)]]
	Set the Light Saturation state value and wait for a response.

	* ``Lvl`` - Saturation level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models hsl saturation-set-unack <Lvl> [TransTime(ms) [Delay(ms)]]
	Set the Light Saturation state value without requesting a response.

	* ``Lvl`` - Saturation level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_hsl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_hsl_cli.c`

.. doxygengroup:: bt_mesh_light_hsl_cli
   :project: nrf
   :members:
