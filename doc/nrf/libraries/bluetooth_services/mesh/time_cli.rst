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

The Bluetooth mesh shell subsystem provides a set of commands to interact with the Time Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_TIME_CLI`

mdl_time instance get-all
	Print all instances of the Time Client model on the device.


mdl_time instance set <elem_idx>
	Select the Time Client model instantiated on the specified element ID.
	This instance will be used in message sending.
	If no model instance is selected, the first model instance found on the device will be used by default.

	* ``elem_idx`` - Element index where the model instance is found.


mdl_time time-get
	Get the current Time Status of a Time Server.


mdl_time time-set <sec> <subsec> <uncertainty> <tai_utc_delta> <time_zone_offset> <is_authority>
	Set the Time Status of a Time Server and wait for a response.

	* ``sec`` - The current TAI time in seconds.
	* ``subsec`` - The sub-seconds time in units of 1/256th second.
	* ``uncertainty`` - Accumulated uncertainty of the mesh Timestamp in milliseconds.
	* ``tai_utc_delta`` - Current TAI-UTC Delta (leap seconds).
	* ``time_zone_offset`` - Current zone offset in 15-minute increments.
	* ``is_authority`` - Reliable TAI source flag.


mdl_time zone-get
	Get the Time Zone status of a Time Server.


mdl_time zone-set <new_offset> <timestamp>
	Schedule a Time Zone change for the Time Server and wait for a response.

	* ``new_offset`` - New zone offset in 15-minute increments.
	* ``timestamp`` - TAI update point for Time Zone Offset.


mdl_time tai-utc-delta-get
	Get the UTC Delta status of a Time Server.


mdl_time tai-utc-delta-set <delta_new> <timestamp>
	Schedule a UTC Delta change for the Timer Server and wait for a response.

	* ``delta_new`` - New TAI-UTC Delta (leap seconds).
	* ``timestamp`` - TAI update point for TAI-UTC Delta.


mdl_time role-get
	Get the Time Role state of a Time Server.


mdl_time role-set <role>
	Set the Time Role state of a Time Server and wait for a response.

	* ``role`` - Time Role to set. Allowed values:
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
