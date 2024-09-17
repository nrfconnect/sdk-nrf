.. _bt_mesh_sensors_readme:

Bluetooth Mesh sensors
######################

.. contents::
   :local:
   :depth: 2

.. note::
   A new sensor API is introduced as of |NCS| v2.6.0.
   The old API is deprecated, but still available by enabling the Kconfig option :kconfig:option:`CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE`.
   The Kconfig option is enabled by default in the deprecation period.
   See the documentation for |NCS| versions prior to v2.6.0 for documentation about the old sensor API.

The Bluetooth® Mesh specification provides a common scheme for representing all sensors.
A single Bluetooth Mesh sensor instance represents a single physical sensor, and a mesh device may present any number of sensors to the network through a Sensor Server model.
Sensors represent their measurements as a list of sensor channels, as described by the sensor's assigned type.

Sensors are accessed through the Sensor models, which are documented separately:

* :ref:`bt_mesh_sensor_models`

  - :ref:`bt_mesh_sensor_srv_readme`
  - :ref:`bt_mesh_sensor_cli_readme`

.. note::
   Several floating point computations are done internally in the stack when using the sensor API.
   It is recommended to enable the :kconfig:option:`CONFIG_FPU` Kconfig option to improve the performance of these computations.

.. _bt_mesh_sensor_basic_example:

Basic example
*************

A sensor reporting the device operating temperature could combine the Bluetooth Mesh :c:var:`Present Device Operating Temperature <bt_mesh_sensor_present_dev_op_temp>` sensor type with the on-chip ``TEMP_NRF5`` temperature sensor driver:

.. code-block:: c

   static const struct device *dev = DEVICE_DT_GET_ONE(nordic_nrf_temp);

   static int temp_get(struct bt_mesh_sensor *sensor,
                       struct bt_mesh_msg_ctx *ctx,
                       struct bt_mesh_sensor_value *rsp)
   {
       struct sensor_value value;
       int err;

       sensor_sample_fetch(dev);
       err = sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &value);
       if (err) {
           return err;
       }
       return bt_mesh_sensor_value_from_sensor_value(&value, rsp);
   }

   struct bt_mesh_sensor temp_sensor = {
       .type = &bt_mesh_sensor_present_dev_op_temp,
       .get = temp_get,
   };

   void init(void)
   {
        __ASSERT(device_is_ready(dev), "Sensor device not ready");
   }

Additionally, a pointer to the ``temp_sensor`` structure should be passed to a Sensor Server to be exposed to the mesh.
See :ref:`bt_mesh_sensor_srv_readme` for details.

.. _bt_mesh_sensor_values:

Sensor values
*************

Sensor values are represented in the API using :c:struct:`bt_mesh_sensor_value`.
This contains the raw sensor value, encoded according to a certain Bluetooth GATT Characteristic, and a pointer to :c:struct:`bt_mesh_sensor_format` describing how that characteristic is encoded/decoded.

Applications will normally not access :c:member:`bt_mesh_sensor_value.raw` or the members of :c:member:`bt_mesh_sensor_value.format` directly.
Instead, API functions for converting between :c:struct:`bt_mesh_sensor_value` and the values suitable for application use are used.
An exception to this is when statically initializing :c:struct:`bt_mesh_sensor_value` at compile-time, in which case the API functions cannot be used.

The sensor API is built to integrate well with the Zephyr :ref:`zephyr:sensor` API, and provides functions for converting to and from :c:struct:`sensor_value`.

.. _bt_mesh_sensor_types:

Sensor types
************

Sensor types are the specification defined data types for the various Bluetooth Mesh sensor parameters.
Each sensor type is assigned its own Device Property ID, as specified in the Bluetooth Mesh device properties specification.
Like the Device Properties, the Sensor types are connected to a Bluetooth GATT Characteristic, which describes the unit, range, resolution and encoding scheme of the sensor type.

.. note::
   The Bluetooth Mesh specification only allows sensor types that have a Device Property ID in the Bluetooth Mesh device properties specification.
   It's not possible to represent vendor specific sensor values.

The sensor types may either be used as the data types of the sensor output values, or as configuration parameters for the sensors.

.. _bt_mesh_sensor_types_channels:

Sensor channels
===============

Each sensor type may consist of one or more channels.
The list of sensor channels in each sensor type is immutable, and all channels must always have a valid value when the sensor data is passed around.
This is slightly different from the sensor type representation in the Bluetooth Mesh specification, which represents multi-channel sensors as structures, rather than flat lists.

Each channel in a sensor type is represented by a single :c:struct:`bt_mesh_sensor_value` structure.
This contains the raw value of the sensor value, and a pointer to :c:struct:`bt_mesh_sensor_format` used for encoding and decoding of the raw value.

Every sensor channel has a name and a unit, as listed in the sensor type documentation.
The name and unit are only available if :kconfig:option:`CONFIG_BT_MESH_SENSOR_LABELS` option is set, and can aid in debugging and presentation of the sensor output.
Both the channel name and unit is also listed in the documentation for each sensor type.

Most sensor values are reported as scalars with some scaling factor applied to them during encoding.
This scaling factor and the encoded data type determines the resolution and range of the sensor data in a specific channel.
For instance, if a sensor channel measuring electric current has a resolution of 0.5 Ampere, this is the highest resolution value other mesh devices will be able to read out from the sensor.
Before encoding, the sensor values are rounded to their nearest available representation, so the following sensor value would be read as 7.5 Ampere:

.. code-block:: c

   struct bt_mesh_sensor_value sensor_val;

   /* Sensor value: 7.3123 A */
   (void)bt_mesh_sensor_value_from_float(
       &bt_mesh_sensor_format_electric_current,
       7.3123f, &sensor_val);

Various other encoding schemes are used to represent non-scalars.
See the documentation or specification for the individual sensor channels for more details.

.. _bt_mesh_sensor_types_series:

Sensor series types
===================

The sensor series functionality may be used for all sensor types.
However, some sensor types are made specifically for being used in a sensor series.
These sensor types have one primary channel containing the sensor data and two secondary channels that denote some interval in which the primary channel's data is captured.
Together, the three channels are able to represent historical sensor data as a histogram, and Sensor Client models may request access to specific measurement spans from a Sensor Server model.

The unit of the measurement span is defined by the sensor type, and will typically be a time interval or a range of operational parameters, like temperature or voltage level.
For instance, the :c:var:`bt_mesh_sensor_rel_dev_energy_use_in_a_period_of_day` sensor type represents the energy used by the device in specific periods of the day.
The primary channel of this sensor type measures energy usage in kWh, and the secondary channels denote the timespan in which the specific energy usage was measured.
A sensor of this type may be queried for specific measurement periods measured in hours, and should provide the registered energy usage only for the requested time span.

.. _bt_mesh_sensor_types_settings:

Sensor setting types
====================

Some sensor types are made specifically to act as sensor settings.
These values are encoded the same way as other sensor types, but typically represent a configurable sensor setting or some specification value assigned to the sensor from the manufacturer.
For instance, the :c:var:`bt_mesh_sensor_motion_threshold` sensor type can be used to configure the sensitivity of a sensor reporting motion sensor data (:c:var:`bt_mesh_sensor_motion_sensed`).

Typically, settings should only be meta data related to the sensor data type, but the API contains no restrictions for which sensor types can be used for sensor settings.

.. _bt_mesh_sensor_types_list:

Available sensor types
======================

All available sensor types are collected in the :ref:`bt_mesh_sensor_types_readme` module.

.. _bt_mesh_sensor_publishing:

Sample data reporting
*********************

Sensors may report their values to the mesh in three ways:

- Unprompted publications
- Periodic publication
- Polling

Unprompted publications may be done at any time, and only includes the sensor data of a single sensor at a time.
The application may generate an unprompted publication by calling :c:func:`bt_mesh_sensor_srv_sample`.
This triggers the sensor's :c:member:`bt_mesh_sensor.get` callback, and only publishes if the sensor's *Delta threshold* is satisfied.

Unprompted publications can also be forced by calling :c:func:`bt_mesh_sensor_srv_pub` directly.

Periodic publication is controlled by the Sensor Server model's publication parameters, and configured by the Config models.
The sensor Server model reports data for all its sensor instances periodically, at a rate determined by the sensors' cadence.
Every publication interval, the Server consolidates a list of sensors to include in the publication, and requests the most recent data from each.
The combined data of all these sensors is published as a single message for other nodes in the mesh network.

If no publication parameters are configured for the Sensor Server model, Sensor Client models may poll the most recent sensor samples directly.

All three methods of reporting may be combined.

.. _bt_mesh_sensor_publishing_cadence:

Cadence
=======

Each sensor may use the cadence state to control the rate at which their data is published.
The sensor's publication interval is defined as a divisor of the holding sensor Server's publication interval that is always a power of two.
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
===============

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
***********

Descriptors are optional meta information structures for every sensor.
A sensor's Descriptor contains parameters that may aid other mesh nodes in interpreting the data:

* Tolerance
* Sampling function
* Measurement period
* Update interval

The sensor descriptor is constant throughout the sensor's lifetime.
If the sensor has a descriptor, a pointer to it should be passed to :c:member:`bt_mesh_sensor.descriptor` on init, as for example done in the code below:

.. code-block:: c

   static const struct bt_mesh_sensor_descriptor temp_sensor_descriptor = {
       .tolerance = {
           .negative = BT_MESH_SENSOR_TOLERANCE_ENCODE(0.75f)
           .positive = BT_MESH_SENSOR_TOLERANCE_ENCODE(3.5f)
       },
       .sampling_type = BT_MESH_SENSOR_SAMPLING_ARITHMETIC_MEAN,
       .period = 300,
       .update_interval = 50
   };

   struct bt_mesh_sensor temp_sensor = {
       .type = &bt_mesh_sensor_present_dev_op_temp,
       .get = temp_get,
       .descriptor = &temp_sensor_descriptor
   };


See :c:struct:`bt_mesh_sensor_descriptor` for details.

.. _bt_mesh_sensor_usage:

Usage
*****

Sensors instances are generally static structures that are initialized at startup.
Only the :c:member:`bt_mesh_sensor.type` member is mandatory, the rest are optional.
Apart from the Cadence and Descriptor states, all states are accessed through getter functions.
The absence of a getter for a state marks it as not supported by the sensor.

Sensor data
===========

Sensor data is accessed through the :c:member:`bt_mesh_sensor.get` callback, which is expected to fill the ``rsp`` parameter with the most recent sensor data and return a status code.
Each sensor channel must be encoded according to the channel format.
This can be done using one of the conversion functions :c:func:`bt_mesh_sensor_value_from_micro`, :c:func:`bt_mesh_sensor_value_from_float` or :c:func:`bt_mesh_sensor_value_from_sensor_value`.
A pointer to the format for a given channel can be found through the :c:struct:`bt_mesh_sensor` pointer passed to the callback in a following way:

.. code-block:: c

   static int get_cb(struct bt_mesh_sensor *sensor,
                       struct bt_mesh_msg_ctx *ctx,
                       struct bt_mesh_sensor_value *rsp)
   {
       /* Get the correct format to use for encoding rsp[0]: */
       const struct_bt_mesh_sensor_format *channel_0_format =
           sensor->type->channels[0].format;
   }

The sensor data in the callback typically comes from a sensor using the :ref:`Zephyr sensor API <zephyr:sensor>`.
The Zephyr sensor API records samples in two steps:

1.
Tell the sensor to take a sample by calling :c:func:`sensor_sample_fetch`.
2.
Read the recorded sample data with :c:func:`sensor_channel_get`.

The first step may be done at any time.
Typically, the sensor fetching is triggered by a timer, an external event or a sensor trigger, but it may be called in the ``get`` callback itself.
Note that the ``get`` callback requires an immediate response, so if the sample fetching takes a significant amount of time, it should generally be done asynchronously.
The method of sampling may be communicated to other mesh nodes through the sensor's :ref:`descriptor <bt_mesh_sensor_descriptors>`.

The read step would typically be done in the callback, to pass the sensor data to the mesh.

If the Sensor Server is configured to do periodic publishing, the ``get`` callback will be called for every publication interval.
Publication may also be forced by calling :c:func:`bt_mesh_sensor_srv_sample`, which will trigger the ``get`` callback and publish only if the sensor value has changed.

Sensor series
=============

Sensor series data can be provided for all sensor types.
To enable the sensor's series data feature, :c:member:`bt_mesh_sensor_series.column_count` must be specified and the sensor series :c:member:`bt_mesh_sensor_series.get` callback must be implemented.

For sensor types with more than two channels, the series data is organized into a static set of columns, specified at init.
The format of the column may be queried with :c:func:`bt_mesh_sensor_column_format_get`.

The ``get`` callback gets called with an index of one of the columns, and is expected to fill the ``value`` parameter with sensor data for the specified column.
If a Sensor Client requests a series of columns, the callback may be called repeatedly, requesting data from each column.

Example: A three-channel sensor (average ambient temperature in a period of day) as a sensor series:

.. code-block:: c

   /* Macro for statically initializing time_decihour_8.
    * Raw is computed by multiplying by 10 according to
    * the resolution specified in the GATT Specification
    * Supplement.
    */
   #define TIME_DECIHOUR_8_INIT(_hours) {                \
       .format = &bt_mesh_sensor_format_time_decihour_8, \
       .raw = { (_hours) * 10 }                          \
   }

   #define COLUMN_INIT(_start, _width) { \
       TIME_DECIHOUR_8_INIT(_start),     \
       TIME_DECIHOUR_8_INIT(_width)      \
   }

   /* 4 columns representing different hours in a day */
   static const struct bt_mesh_sensor_column columns[] = {
       COLUMN_INIT(0, 6),
       COLUMN_INIT(6, 6),
       COLUMN_INIT(12, 6),
       COLUMN_INIT(18, 6)
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
   static float avg_temp[ARRAY_SIZE(columns)];

   static int getter(struct bt_mesh_sensor *sensor, struct bt_mesh_msg_ctx *ctx,
                     uint32_t column_index, struct bt_mesh_sensor_value *value)
   {
       int err = bt_mesh_sensor_value_from_float(
           sensor->type->channels[0].format, &avg_temp[column_index], &value[0]);

       if (err) {
           return err;
       }
       value[1] = columns[column_index].start;

       /* Compute end value from column start and width: */
       int64_t start, width;
       enum bt_mesh_sensor_value_status status;

       status = bt_mesh_sensor_value_to_micro(&columns[column_index].start, &start);
       if (!bt_mesh_sensor_status_is_numeric(status)) {
           return -EINVAL;
       }
       status = bt_mesh_sensor_value_to_micro(&columns[column_index].width, &width);
       if (!bt_mesh_sensor_value_status_is_numeric(status)) {
           return -EINVAL;
       }
       return bt_mesh_sensor_value_from_micro(
           bt_mesh_sensor_column_format_get(sensor),
           start + width, &value[2]);
   }

Example: Single-channel sensor (motion sensed) as a sensor series:

.. code-block:: c

   #define COLUMN_COUNT 10

   static struct bt_mesh_sensor motion_sensor = {
       .type = &bt_mesh_sensor_motion_sensed,
       .series = {
            /* Note: no column array necessary for 1 or 2 channel sensors */
            .column_count = COLUMN_COUNT,
            .get = getter,
        },
   };

   /** Sensor data is divided into columns and filled elsewhere */
   static uint8_t motion[COLUMN_COUNT];

   static int getter(struct bt_mesh_sensor *sensor, struct bt_mesh_msg_ctx *ctx,
                     uint32_t column_index, struct bt_mesh_sensor_value *value)
   {
       return bt_mesh_sensor_value_from_micro(
           sensor->type->channels[0].format,
           motion[column_index] * 1000000LL, &value[0]);
   }

Sensor settings
===============

The list of settings a sensor supports should be set on init.
The list should be constant throughout the sensor's lifetime, and may be declared ``const``.
Each entry in the list has a type and two access callbacks, and the list should only contain unique entry types.

The :c:member:`bt_mesh_sensor_setting.get` callback is mandatory, while the :c:member:`bt_mesh_sensor_setting.set` is optional, allowing for read-only entries.
The value of the settings may change at runtime, even outside the ``set`` callback.
New values may be rejected by returning a negative error code from the ``set`` callback.
The following code is an example of adding a setting to a sensor:

.. code-block:: c

   static void motion_threshold_get(struct bt_mesh_sensor_srv *srv,
                                    struct bt_mesh_sensor *sensor,
                                    const struct bt_mesh_sensor_setting *setting,
                                    struct bt_mesh_msg_ctx *ctx,
                                    struct bt_mesh_sensor_value *rsp)
   {
        /** Get the current threshold in an application defined way and
         *  store it in rsp.
         */
        get_threshold(rsp);
   }

   static int motion_threshold_set(struct bt_mesh_sensor_srv *srv,
                                   struct bt_mesh_sensor *sensor,
                                   const struct bt_mesh_sensor_setting *setting,
                                   struct bt_mesh_msg_ctx *ctx,
                                   const struct bt_mesh_sensor_value *value)
   {
        /** Store incoming threshold in application-defined way.
         *  Return error code to reject set.
         */
        return set_threshold(value);
   }

   static const struct bt_mesh_sensor_setting settings[] = {
       {
           .type = &bt_mesh_sensor_motion_threshold,
           .get = motion_threshold_get,
           .set = motion_threshold_set,
       }
   };

   static struct bt_mesh_sensor motion_sensor = {
       .type = &bt_mesh_sensor_motion_sensed,
       .get = get_motion,
       .settings = {
           .list = settings,
           .count = ARRAY_SIZE(settings)
        }
   };

.. _bt_mesh_sensor_api:

API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/sensor.h`
| Source file: :file:`subsys/bluetooth/mesh/sensor.c`

.. doxygengroup:: bt_mesh_sensor
