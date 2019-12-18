.. _bt_mesh_sensors_readme:

Bluetooth Mesh Sensors
######################

The Bluetooth Mesh specification provides a common scheme for representing all sensors.
A single Bluetooth Mesh Sensor instance represents a single physical sensor, and a mesh device may present any number of sensors to the network through a Sensor Server model.
Sensors represent their measurements as a list of sensor channels, as described by the sensor's assigned type.

Topics on Bluetooth Mesh Sensors covered in this document:

* :ref:`bt_mesh_sensor_basic_example`
* :ref:`bt_mesh_sensor_types`

  - :ref:`bt_mesh_sensor_types_channels`
  - :ref:`bt_mesh_sensor_types_series`
  - :ref:`bt_mesh_sensor_types_settings`
  - :ref:`bt_mesh_sensor_types_list`

* :ref:`bt_mesh_sensor_publishing`

  - :ref:`bt_mesh_sensor_publishing_cadence`
  - :ref:`bt_mesh_sensor_publishing_delta`

* :ref:`bt_mesh_sensor_descriptors`
* :ref:`bt_mesh_sensor_usage`
* :ref:`bt_mesh_sensor_api`

Sensors are accessed through the Sensor models, which are documented separately:

* :ref:`bt_mesh_sensor_models`

  - :ref:`bt_mesh_sensor_srv_readme`
  - :ref:`bt_mesh_sensor_cli_readme`

.. _bt_mesh_sensor_basic_example:

Basic example
=============

A sensor reporting the device operating temperature could combine the Bluetooth Mesh :cpp:var:`Present Device Operating Temperature <bt_mesh_sensor_present_dev_op_temp>` sensor type with the on-chip ``TEMP_NRF5`` temperature sensor driver:

.. code-block:: c

   static struct device *dev;

   static int temp_get(struct bt_mesh_sensor *sensor,
                       struct bt_mesh_msg_ctx *ctx,
                       struct sensor_value *rsp)
   {
       sensor_sample_fetch(dev);
       return sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, rsp);
   }

   struct bt_mesh_sensor temp_sensor = {
       .type = &bt_mesh_sensor_present_dev_op_temp,
       .get = temp_get,
   };

   void init(void)
   {
       dev = device_get_binding(DT_INST_0_NORDIC_NRF_TEMP_LABEL);
   }

Additionally, a pointer to the ``temp_sensor`` structure should be passed to a Sensor Server to be exposed to the mesh.
See :ref:`bt_mesh_sensor_srv_readme` for details.

.. _bt_mesh_sensor_types:

Sensor types
============

Sensor types are the specification defined data types for the various Bluetooth Mesh sensor parameters.
Each sensor type is assigned its own Device Property ID, as specified in the Bluetooth Mesh Device Properties Specification.
Like the Device Properties, the Sensor types are connected to a Bluetooth GATT Characteristic, which describes the unit, range, resolution and encoding scheme of the sensor type.

.. note::
   The Bluetooth Mesh specification only allows sensor types that have a Device Property ID in the Bluetooth Mesh Device Properties Specification. It's not possible to represent vendor specific sensor values.

The sensor types may either be used as the data types of the sensor output values, or as configuration parameters for the sensors.

The Bluetooth Mesh Sensor type API is built to mirror and integrate well with the Zephyr :ref:`zephyr:sensor` API.
Some concepts in the Bluetooth Mesh Specification are changed slightly to fit better with the Zephyr Sensor API, with focus on making integration as simple as possible.

.. _bt_mesh_sensor_types_channels:

Sensor Channels
***************

Each sensor type may consist of one or more channels.
The list of sensor channels in each sensor type is immutable, and all channels must always have a valid value when the sensor data is passed around.
This is slightly different from the sensor type representation in the Bluetooth Mesh Specification, which represents multi-channel sensors as structures, rather than flat lists.

Each channel in a sensor type is represented by a single :c:type:`sensor_value`.
For sensor values that are represented as whole numbers, the fractional part of the value (:cpp:member:`sensor_value::val2`) is ignored.
Boolean types are inferred only from the integer part of the value (:cpp:member:`sensor_value::val1`).

Every sensor channel has a name and a unit, as listed in the sensor type documentation.
The name and unit are only available if :option:`CONFIG_BT_MESH_SENSOR_LABELS` option is set, and can aid in debugging and presentation of the sensor output.
Both the channel name and unit is also listed in the documentation for each sensor type.

Most sensor values are reported as scalars with some scaling factor applied to
them during encoding. This scaling factor and the encoded data type determines
the resolution and range of the sensor data in a specific channel. For instance,
if a sensor channel measuring electric current has a resolution of 0.5 Ampere,
this is the highest resolution value other mesh devices will be able to read out
from the sensor. Before encoding, the sensor values are rounded to their nearest
available representation, so the following sensor value would be read as 7.5
Ampere:

.. code-block:: c

   /* Sensor value: 7.3123 A */
   struct sensor_value electrical_current = {
       .val1 = 7,
       .val2 = 312300, /* 6 digit fraction */
   };

Various other encoding schemes are used to represent non-scalars.
See the documentation or specification for the individual sensor channels for more details.

.. _bt_mesh_sensor_types_series:

Sensor series types
*******************

Some sensor types are made specially for being used in a sensor series.
These sensor types have one primary channel containing the sensor data and two secondary channels that denote some interval in which the primary channel's data is captured.
Together, the three channels are able to represent historical sensor data as a histogram, and Sensor Client models may request access to specific measurement spans from a Sensor Server model.

The unit of the measurement span is defined by the sensor type, and will typically be a time interval or a range of operational parameters, like temperature or voltage level.
For instance, the :cpp:var:`bt_mesh_sensor_rel_dev_energy_use_in_a_period_of_day` sensor type represents the energy used by the device in specific periods of the day.
The primary channel of this sensor type measures energy usage in kWh, and the secondary channels denote the timespan in which the specific energy usage was measured.
A sensor of this type may be queried for specific measurement periods measured in hours, and should provide the registered energy usage only for the requested time span.

.. _bt_mesh_sensor_types_settings:

Sensor setting types
********************

Some sensor types are made specifically to act as sensor settings.
These values are encoded the same way as other sensor types, but typically represent a configurable sensor setting or some specification value assigned to the sensor from the manufacturer.
For instance, the :cpp:var:`bt_mesh_sensor_motion_threshold` sensor type can be used to configure the sensitivity of a sensor reporting motion sensor data (:cpp:var:`bt_mesh_sensor_motion_sensed`).

Typically, settings should only be meta data related to the sensor data type, but the API contains no restrictions for which sensor types can be used for sensor settings.

.. _bt_mesh_sensor_types_list:

Available sensor types
**********************

All available sensor types are collected in a single module:

.. toctree::
   :maxdepth: 1

   sensor_types.rst

.. _bt_mesh_sensor_publishing:

Sample data reporting
=====================

Sensors may report their values to the mesh in three ways:

- Unprompted publications
- Periodic publication
- Polling

Unprompted publications may be done at any time, and only includes the sensor data of a single sensor at a time.
The application may generate an unprompted publication by calling :cpp:func:`bt_mesh_sensor_srv_sample`.
This triggers the sensor's :cpp:member:`bt_mesh_sensor::get` callback, and only publishes if the sensor's *Delta threshold* is satisfied.

Unprompted publications can also be forced by calling :cpp:func:`bt_mesh_sensor_srv_pub` directly.

Periodic publication is controlled by the Sensor Server model's publication parameters, and configured by the Config models.
The sensor Server model reports data for all its sensor instances periodically, at a rate determined by the sensors' cadence.
Every publication interval, the Server consolidates a list of sensors to include in the publication, and requests the most recent data from each.
The combined data of all these sensors is published as a single message for other nodes in the mesh network.

If no publication parameters are configured for the Sensor Server model, Sensor Client models may poll the most recent sensor samples directly.

All three methods of reporting may be combined.

.. _bt_mesh_sensor_publishing_cadence:

Cadence
*******

Each sensor may use the cadence state to control the rate at which their data is published.
The sensor's publication interval is defined as a divisor of the holding sensor Server's publication interval, that is always a power of two.
Under normal circumstances, the sensor's period divisor is always 1, and the sensor only publishes on the Server's actual publication interval.

All single-channel sensors have a configurable *fast cadence* range that automatically controls the sensor cadence.
If the sensor's value is within its configured fast cadence range, the sensor engages the period divisor, and starts publishing with fast cadence.

The fast cadence range always starts at the cadence range ``low`` value, and spans to the cadence range ``high`` value.
If the ``high`` value is lower than the ``low`` value, the effect is inverted, and the sensor operates at high cadence if its value is *outside* the range.

To prevent sensors from saturating the mesh network, each sensor also defines a minimum publication interval, which is always taken into account when performing the period division.

The period divisor, fast cadence range and minimum interval is configured by a Sensor Client model (through a Sensor Setup Server).
The sensor's cadence is automatically recalculated for every sample, based on its configuration.

.. _bt_mesh_sensor_publishing_delta:

Delta threshold
***************

All single channel sensors have a delta threshold state to aid the publication rate.
The delta threshold state determines the smallest change in sensor value that should trigger a publication.
Whenever a sensor value is published to the mesh network (through periodic publishing or otherwise), the sensor saves the value, and compares it to subsequent samples.
Once a sample is sufficiently far away from the previously published value, it gets published.

The delta threshold works on both periodic publication and unprompted publications.
If periodic publication is enabled and the minimum interval has expired, the
sensor will periodically check whether the delta threshold has been breached, so that it can publish the value on the next periodic interval.

The delta threshold may either be specified as a percent wise change, or as an absolute delta.
The percent wise change is always measured relatively to the previously published value, and allows the sensor to automatically scale its threshold to account for relative inaccuracy or noise.

The sensor has separate delta thresholds for positive and negative changes.

.. _bt_mesh_sensor_descriptors:

Descriptors
============

Descriptors are optional meta information structures for every sensor.
A sensor's Descriptor contains parameters that may aid other mesh nodes in interpreting the data:

* Tolerance
* Sampling function
* Measurement period
* Update interval

The sensor descriptor is constant throughout the sensor's lifetime. If the sensor has a descriptor, a pointer to it should be passed to :cpp:member:`bt_mesh_sensor::descriptor` on init.

See :cpp:type:`bt_mesh_sensor_descriptor` for details.

.. _bt_mesh_sensor_usage:

Usage
=====

Sensors instances are generally static structures that are initialized at startup.
Only the :cpp:member:`bt_mesh_sensor::type` member is mandatory, the rest are optional.
Apart from the Cadence and Descriptor states, all states are accessed through getter functions.
The absence of a getter for a state marks it as not supported by the sensor.

Sensor data
***********

Sensor data is accessed through the :cpp:member:`bt_mesh_sensor::get` callback, which is expected to fill the ``rsp`` parameter with the most recent sensor data and return a status code.
Each sensor channel will be encoded internally according to the sensor type.

The sensor data in the callback typically comes from a sensor using the :ref:`Zephyr sensor API <zephyr:sensor>`.
The Zephyr sensor API records samples in two steps:

1. Tell the sensor to take a sample by calling :cpp:func:`sensor_sample_fetch`.
2. Read the recorded sample data with :cpp:func:`sensor_channel_get`.

The first step may be done at any time.
Typically, the sensor fetching is triggered by a timer, an external event or a sensor trigger, but it may be called in the ``get`` callback itself.
Note that the ``get`` callback requires an immediate response, so if the sample fetching takes a significant amount of time, it should generally be done asynchronously.
The method of sampling may be communicated to other mesh nodes through the sensor's :ref:`descriptor <bt_mesh_sensor_descriptors>`.

The read step would typically be done in the callback, to pass the sensor data to the mesh.

If the Sensor Server is configured to do periodic publishing, the ``get`` callback will be called for every publication interval. Publication may also be forced by calling :cpp:func:`bt_mesh_sensor_srv_sample`, which will trigger the ``get`` callback and publish only if the sensor value has changed.

Sensor series
*************

Sensor series data is organized into a static set of columns, specified at init.
The sensor series :cpp:member:`bt_mesh_sensor_series::get` callback must be implemented to enable the sensor's series data feature.
Only some sensor types support series access, see the sensor type's documentation.
The format of the column may be queried with :cpp:func:`bt_mesh_sensor_column_format_get`.

The ``get`` callback gets called with a direct pointer to one of the columns in the column list, and is expected to fill the ``value`` parameter with sensor data for the specified column. If a Sensor Client requests a series of columns, the callback may be called repeatedly, requesting data from each column.

Example: Average ambient temperature in a period of day as a sensor series:

.. code-block:: c

   /* 4 columns representing different hours in a day */
   static const struct bt_mesh_sensor_column columns[] = {
       {{0}, {6}},
       {{6}, {12}},
       {{12}, {18}},
       {{18}, {24}},
   };

   static struct bt_mesh_sensor temp_sensor = {
       .type = &bt_mesh_sensor_avg_amb_temp_in_day,
       .series = {
           columns,
           ARRAY_SIZE(columns),
           getter,
       },
   };

   /** Sensor data is divided into columns and filled elsewhere */
   static struct sensor_value avg_temp[ARRAY_SIZE(columns)];

   static int getter(struct bt_mesh_sensor *sensor, struct bt_mesh_msg_ctx *ctx,
		                 const struct bt_mesh_sensor_column *column,
		                 struct sensor_value *value)
   {
       /* The column pointer is always a direct pointer to one of our columns,
        *  so determining the column index is easy:
        */
       u32_t index = column - &columns[0];

       value[0] = avg_temp[index];
       value[1] = column->start;
       value[2] = column->end;

       return 0;
   }

Sensor settings
***************

The list of settings a sensor supports should be set on init. The list should be constant throughout the sensor's lifetime, and may be declared ``const``.
Each entry in the list has a type and two access callbacks, and the list should only contain unique entry types.

The :cpp:member:`bt_mesh_sensor_setting::get` callback is mandatory, while the :cpp:member:`bt_mesh_sensor_setting::set` is optional, allowing for read-only entries. The value of the settings may change at runtime, even outside the ``set`` callback. New values may be rejected by returning a negative error code from the ``set`` callback.

.. _bt_mesh_sensor_api:

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/sensor.h`
| Source file: :file:`subsys/bluetooth/mesh/sensor.c`

.. doxygengroup:: bt_mesh_sensor
   :project: nrf
   :members:
