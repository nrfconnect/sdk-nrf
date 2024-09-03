.. _bt_mesh_time_cli_readme:

Time Client
###########

.. contents::
   :local:
   :depth: 2

The Time Client model provides time sources for and configures :ref:`bt_mesh_time_srv_readme` models.

Unlike the Time Server model, the Time Client only creates a single model instance in the mesh composition data.
The Time Client can send messages to both the Time Server and the Time Setup Server, as long as it has the right application keys.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth Mesh shell subsystem provides a set of commands to interact with the Time Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_TIME_CLI`

mesh models time instance get-all
	Print all instances of the Time Client model on the device.


mesh models time instance set <ElemIdx>
	Select the Time Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``ElemIdx`` - Element index where the model instance is found.


mesh models time time-get
	Get the current Time Status of a Time Server.


mesh models time time-set <Sec> <Subsec> <Uncertainty> <TaiUtcDlt> <TimeZoneOffset> <IsAuthority>
	Set the Time Status of a Time Server and wait for a response.

	* ``Sec`` - The current TAI time in seconds.
	* ``Subsec`` - The sub-seconds time in units of 1/256th second.
	* ``Uncertainty`` - Accumulated uncertainty of the mesh Timestamp in milliseconds.
	* ``TaiUtcDlt`` - Current TAI-UTC Delta (leap seconds).
	* ``TimeZoneOffset`` - Current zone offset in 15-minute increments.
	* ``IsAuthority`` - Reliable TAI source flag.


mesh models time zone-get
	Get the Time Zone status of a Time Server.


mesh models time zone-set <NewOffset> <Timestamp>
	Schedule a Time Zone change for the Time Server and wait for a response.

	* ``NewOffset`` - New zone offset in 15-minute increments.
	* ``Timestamp`` - TAI update point for Time Zone Offset.


mesh models time tai-utc-delta-get
	Get the UTC Delta status of a Time Server.


mesh models time tai-utc-delta-set <DltNew> <Timestamp>
	Schedule a UTC Delta change for the Timer Server and wait for a response.

	* ``DltNew`` - New TAI-UTC Delta (leap seconds).
	* ``Timestamp`` - TAI update point for TAI-UTC Delta.


mesh models time role-get
	Get the Time Role state of a Time Server.


mesh models time role-set <Role>
	Set the Time Role state of a Time Server and wait for a response.

	* ``Role`` - Time Role to set. Allowed values:
		* ``0`` - The element does not participate in propagation of time information.
		* ``1`` - The element publishes Time Status messages but does not process received Time Status messages.
		* ``2`` - The element both publishes and processes received Time Status messages.
		* ``3`` - The element does not publish but processes received Time Status messages.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/time_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/time_cli.c`

.. doxygengroup:: bt_mesh_time_cli
   :project: nrf
   :members:
