.. _bt_mesh_sensor_cli_readme:

Sensor Client
#############

.. contents::
   :local:
   :depth: 2

The Sensor Client model reads and configures the sensors exposed by :ref:`bt_mesh_sensor_srv_readme` models.

Unlike the Sensor Server model, the Sensor Client only creates a single model instance in the mesh composition data.
The Sensor Client can send messages to both the Sensor Server and the Sensor Setup Server, as long as it has the right application keys.

Configuration
*************

The Sensor Client will receive data from the Sensor Servers asynchronously.
In most cases, the incoming data needs to be interpreted differently based on the type of sensor that produced it.

When parsing incoming messages, the Sensor Client is required to do a lookup of the sensor type of the data so it can decode it correctly.
For this purpose, the :ref:`bt_mesh_sensor_types` module includes the :ref:`list of available sensor types <bt_mesh_sensor_types_readme>`.

By default, all sensor types are available when the Sensor Client model is compiled in, but this behavior can be overridden to reduce flash consumption by explicitly disabling :kconfig:option:`CONFIG_BT_MESH_SENSOR_ALL_TYPES`.
In this case, only the referenced sensor types will be available.

.. note::
    Whenever the Sensor Client receives a sensor type that it is unable to interpret, it calls its :c:member:`bt_mesh_sensor_cli_handlers.unknown_type` callback.
    The Sensor Client API is designed to force the application to reference any sensor types it wants to communicate with, so this issue will commonly not occur.

The Sensor Client API supports both blocking functions and asynchronous callbacks for accessing the Sensor Server data.

Extended models
***************

None.

Persistent storage
******************

None.

Shell commands
**************

The Bluetooth Mesh shell subsystem provides a set of commands to interact with the Sensor Client model instantiated on a device.

To make these commands available, enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_MESH_SHELL`
* :kconfig:option:`CONFIG_BT_MESH_SHELL_SENSOR_CLI`

mesh models sensor instance get-all
   Print all instances of the Sensor Client model on the device.


mesh models sensor instance set <ElemIdx>
   Select the Sensor Client model instantiated on the specified element ID.
   This instance will be used in message sending.
   If no model instance is selected, the first model instance found on the device will be used by default.

   * ``ElemIdx`` - Element index where the model instance is found.


mesh models sensor desc-get [SensorID]
   Get the sensor descriptor for one or all sensors on the server.
   This will get a maximum number of sensor descriptors specified by :kconfig:option:`CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SENSORS`.

   * ``SensorID`` - If present, selects the sensor for which to get the descriptor.


mesh models sensor cadence-get <SensorID>
   Get the configured cadence for a sensor on the server.

   * ``SensorID`` - Selects the sensor for which to get the configured cadence.


mesh models sensor cadence-set <SensorID> <FastPerDiv> <MinInt> <DltType> <DltUp> <DltDown> <CadInside> <RngLow> <RngHigh>
   Set the cadence for a sensor on the server and wait for a response.

   * ``SensorID`` - Selects the sensor for which to get the configured cadence.
   * ``FastPerDiv`` - Divisor for computing fast cadence. Fast period is publish_period / (1 << FastPerDiv).
   * ``MinInt`` - Minimum publish interval in fast region. Interval is never lower than 1 << MinInt.
   * ``DltType`` - Sets the type of delta triggering. 0 = value-based threshold. 1 = percentage-based threshold.
   * ``DltUp`` - Minimum positive delta which triggers publication.
   * ``DltDown`` - Minimum negative delta which triggers publication.
   * ``CadInside`` - Sets the cadence used inside the range. 0 = normal cadence inside, fast outside. 1 = fast cadence inside, normal outside.
   * ``RngLow`` - Lower bound of the cadence range.
   * ``RngHigh`` - Upper bound of the cadence range.


mesh models sensor cadence-set-unack <SensorID> <FastPerDiv> <MinInt> <DltType> <DltUp> <DltDown> <CadInside> <RngLow> <RngHigh>
   Set the cadence for a sensor on the server without waiting for a response.

   * ``SensorID`` - Selects the sensor for which to get the configured cadence.
   * ``FastPerDiv`` - Divisor for computing fast cadence. Fast period is publish_period / (1 << FastPerDiv).
   * ``MinInt`` - Minimum publish interval in fast region. Interval is never lower than 1 << MinInt.
   * ``DltType`` - Sets the type of delta triggering. 0 = value-based threshold. 1 = percentage-based threshold.
   * ``DltUp`` - Minimum positive delta which triggers publication.
   * ``DltDown`` - Minimum negative delta which triggers publication.
   * ``CadInside`` - Sets the cadence used inside the range. 0 = normal cadence inside, fast outside. 1 = fast cadence inside, normal outside.
   * ``RngLow`` - Lower bound of the cadence range.
   * ``RngHigh`` - Upper bound of the cadence range.


mesh models sensor settings-get <SensorID>
   Get the available settings for a sensor on the server.
   This will get a maximum number of settings specified by :kconfig:option:`CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SETTINGS`.

   * ``SensorID`` - Selects the sensor for which to get the available settings.


mesh models sensor setting-get <SensorID> <SettingID>
   Get the value of a setting for a sensor on the server.

   * ``SensorID`` - Selects the sensor for which to get the setting value.
   * ``SettingID`` - Selects the setting to get.


mesh models sensor setting-set <SensorID> <SettingID> <Value>
   Set the value of a setting for a sensor on the server and wait for a response.

   * ``SensorID`` - Selects the sensor for which to set the setting value.
   * ``SettingID`` - Selects the setting to set.
   * ``Value`` - The new value of the setting.


mesh models sensor setting-set-unack <SensorID> <SettingID> <Value>
   Set the value of a setting for a sensor on the server without waiting for a response.

   * ``SensorID`` - Selects the sensor for which to set the setting value.
   * ``SettingID`` - Selects the setting to set.
   * ``Value`` - The new value of the setting.


mesh models sensor get [SensorID]
   Get the sensor value for one or all of the sensors on the server.
   This will get a maximum number of sensor values specified by :kconfig:option:`CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_SENSORS`.

   * ``SensorID`` - If present, selects the sensor for which to get the sensor value.


mesh models sensor series-entry get <SensorID> <Column>
   Get the value of a column for a sensor on the server.

   * ``SensorID`` - Selects the sensor for which to get the entry value.
   * ``Column`` - For sensors with three channels: Start value of the column for which to get the entry value. For other sensors: Index of the column for which to get the entry value.


mesh models sensor series-entries-get <SensorID> [<RngStart> <RngEnd>]
   Get the entries for all columns, or a specified range of columns, for a sensor on the server.
   This will get a maximum number of entries specified by :kconfig:option:`CONFIG_BT_MESH_SHELL_SENSOR_CLI_MAX_COLUMNS`.

   * ``SensorID`` - Selects the sensor for which to get the entries.
   * ``RngStart`` - If present, selects the start of the column range to get.
   * ``RngEnd`` - If present, selects the end of the column range to get. If ``RngStart`` is present, this must also be present.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/sensor_cli.h`
| Source file: :file:`subsys/bluetooth/mesh/sensor_cli.c`

.. doxygengroup:: bt_mesh_sensor_cli
   :project: nrf
   :members:
