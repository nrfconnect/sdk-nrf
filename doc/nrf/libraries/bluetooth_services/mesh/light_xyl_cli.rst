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

mesh models xyl instance get-all
	Print all instances of the Light xyL Client model on the device.


mesh models xyl instance set <ElemIdx>
	Select the Light xyL Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models xyl get
	Get the Light xyL state value of the bound server.


mesh models xyl set <Light> <X> <Y> [TransTime(ms) [Delay(ms)]]
	Set the Light xyL state value and wait for a response.

	* ``Light`` - Lightness level to set.
	* ``X`` - x level to set.
	* ``Y`` - y level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models xyl set-unack <Light> <X> <Y> [TransTime(ms) [Delay(ms)]]
	Set the Light xyL state value without requesting a response.

	* ``Light`` - Lightness level to set.
	* ``X`` - x level to set.
	* ``Y`` - y level to set.
	* ``TransTime`` - If present, defines the transition time in the message in milliseconds.
	* ``Delay`` - If present, defines the delay in the message in milliseconds.


mesh models xyl target-get
	Get the Light xyL target state value of the bound server.


mesh models xyl default-get
	Get the default xyL value of the bound server.


mesh models xyl default-set <Light> <X> <Y>
	Set the default xyL value of the server and wait for a response.

	* ``Light`` - Default lightness level to be set.
	* ``X`` - Default x level to be set.
	* ``Y`` - Default y level to be set.


mesh models xyl default-set-unack <Light> <X> <Y>
	Set the Default xyL state value of the server without requesting a response.

	* ``Light`` - Default lightness level to be set.
	* ``X`` - Default x level to be set.
	* ``Y`` - Default y level to be set.


mesh models xyl range-get
	Get the Light xyL Range state value of the bound server.


mesh models xyl range-set <XMin> <YMin> <XMax> <YMax>
	Set the Light xyL Range state value and wait for a response.

	* ``XMin`` - Minimum allowed x value.
	* ``YMin`` - Minimum allowed y value.
	* ``XMax`` - Maximum allowed x value.
	* ``YMax`` - Maximum allowed y value.


mesh models xyl range-set-unack <XMin> <YMin> <XMax> <YMax>
	Set the Light xyL Range state value without requesting a response.

	* ``XMin`` - Minimum allowed x value.
	* ``YMin`` - Minimum allowed y value.
	* ``XMax`` - Maximum allowed x value.
	* ``YMax`` - Maximum allowed y value.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_xyl_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/light_xyl_cli.c`

.. doxygengroup:: bt_mesh_light_xyl_cli
   :project: nrf
   :members:
