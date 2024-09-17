.. _asset_tracker_v2_sensor_module:

Sensor module
#############

.. contents::
   :local:
   :depth: 2

The sensor module interacts with external sensors present on the `Thingy:91 <Thingy\:91 product page_>`_ and Thingy:91 X.
It collects environmental data and detects motion over a set threshold value.

Features
********

This section documents the various features implemented by the module.

Sensor types
============

The following table lists the sensors and sensor data types supported by the module:

+-------------------------+-----------------+
| Sensor data type        | External device |
+=========================+=================+
| Humidity                | `BME680`_       |
+-------------------------+-----------------+
| Temperature             | `BME680`_       |
+-------------------------+-----------------+
| Atmospheric Pressure    | `BME680`_       |
+-------------------------+-----------------+
| Air Quality (BSEC only) | `BME680`_       |
+-------------------------+-----------------+
| Acceleration (Activity) | `ADXL362`_      |
+-------------------------+-----------------+
| Acceleration (Impact)   | `ADXL372`_      |
+-------------------------+-----------------+

The module controls and collects data from the sensors by interacting with their :ref:`device drivers <device_model_api>` using :ref:`Zephyr's generic sensor API <sensor>`.

Thingy:91 X has a `BME688`_ gas sensor and `ADXL367`_ motion sensor that can be used by the :ref:`asset_tracker_v2_sensor_module` module.

Data sampling
=============

When the module receives an :c:enum:`APP_EVT_DATA_GET` event and the :c:enum:`APP_DATA_ENVIRONMENTAL` type is present in the ``app_data`` list carried in the event, it will sample data.
When data sampling has been carried out, the :c:enum:`SENSOR_EVT_ENVIRONMENTAL_DATA_READY` event is sent from the module with the sampled environmental sensor values.

.. note::
   An nRF91 Series DK does not have any external sensors and battery fuel gauge.
   If the sensor module is queried for sensor data when building for the DK, the event :c:enum:`SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED` is sent out by the module
   upon data sampling.
   For battery fuel gauge data, :c:enum:`SENSOR_EVT_FUEL_GAUGE_NOT_SUPPORTED` is sent.

Motion activity detection
=========================

Motion activity is detected when acceleration in either X, Y or Z plane exceeds the configured activity threshold value.
Stillness is detected when the acceleration is less than the configured inactivity threshold value for the duration of the inactivity time.
The threshold values are set in one of the following two ways:

* When receiving the :c:enum:`DATA_EVT_CONFIG_INIT` event after boot.
  This event contains the default threshold values set by the options :ref:`CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD>` and :ref:`CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD <CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD>` or retrieved from flash.
* When receiving the :c:enum:`DATA_EVT_CONFIG_READY` event.
  This occurs when a new threshold value has been updated from cloud.

Both events contain upper and lower accelerometer threshold values ``accelerometer_activity_threshold`` and ``accelerometer_inactivity_threshold`` in m/s2, present in the event structure.
Further, they contain a timeout value ``accelerometer_inactivity_timeout`` in seconds.

Motion detection is enabled and disabled according to the device mode parameter, received in the configuration events.
It is enabled in the passive mode and disabled in the active mode.
Data sampling requests are sent out both on activity events and inactivity events.

The sensor module sends out a :c:enum:`SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED` event if it detects movement.
Similarly, :c:enum:`SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED` is sent out if there is no movement within the configured timeout.

.. note::
   The DK does not have an external accelerometer.
   However, you can use **Button 2** on the DK to trigger movement for testing purposes.

.. note::
   The accelerometer available on the Thingy:91 needs detailed tuning for each use case to determine reliably which readings are considered as motion.
   This is beyond the scope of the general asset tracker framework this application provides.
   Therefore, the readings are not transmitted to the cloud and are only used to detect a binary active and inactive state.

.. _motion_impact_detection:

Motion impact detection
=======================

Motion impact is detected when the magnitude (root sum squared) of acceleration exceeds the configured threshold value.
To enable motion impact detection, you must include :ref:`CONFIG_EXTERNAL_SENSORS_IMPACT_DETECTION <CONFIG_EXTERNAL_SENSORS_IMPACT_DETECTION>` when building the application.

The threshold is configured using the :kconfig:option:`CONFIG_ADXL372_ACTIVITY_THRESHOLD` option.
The accelerometer records acceleration magnitude when it is in the active mode and reports the peak magnitude once it reverts to the inactive mode.
The accelerometer changes to active mode when the activity threshold is exceeded and reverts to inactive mode once acceleration stays below
:kconfig:option:`CONFIG_ADXL372_INACTIVITY_THRESHOLD` for the duration specified in the :kconfig:option:`CONFIG_ADXL372_INACTIVITY_TIME` option.

When an impact has been detected, a :c:enum:`SENSOR_EVT_MOVEMENT_IMPACT_DETECTED` event is sent from the sensor module.

.. note::
   Impact detection is not implemented for Thingy:91 X.

.. _bosch_software_environmental_cluster_library:

Bosch Software Environmental Cluster (BSEC) library
===================================================

The sensor module supports integration with the BSEC signal processing library using the external sensors, internal convenience API.
If enabled, the BSEC library is used instead of the BME680 Zephyr driver to provide sensor readings from the BME680 for temperature, humidity, and atmospheric pressure.
In addition, the BSEC driver provides an additional sensor reading, indoor air quality (IAQ), which is a metric given in between 0-500 range, which estimates the air quality of the environment.
In the beginning, the IAQ shows 50 (good air), but it is automatically calibrated over time.

.. note::
   Using the BSEC library requires accepting a separate license agreement.
   For details, see :ref:`bme68x_iaq`.

Perform the following steps to enable BSEC:

1. To disable the Zephyr BME680 driver, set the :kconfig:option:`CONFIG_BME680` Kconfig option to false.
#. To enable the external sensors API BSEC integration layer, use the :ref:`CONFIG_BME68X_IAQ <CONFIG_BME68X_IAQ>` Kconfig option.

Air quality readings are provided with the :c:enum:`SENSOR_EVT_ENVIRONMENTAL_DATA_READY` event.

To check and configure the BSEC configuration options, see :ref:`external_sensor_API_BSEC_configurations` section.

Configuration options
*********************

.. _CONFIG_SENSOR_THREAD_STACK_SIZE:

CONFIG_SENSOR_THREAD_STACK_SIZE - Sensor module thread stack size
   This option configures the sensor module's internal thread stack size.

.. _CONFIG_EXTERNAL_SENSORS_IMPACT_DETECTION:

CONFIG_EXTERNAL_SENSORS_IMPACT_DETECTION
   This configuration option enables the impact detection feature.

.. _external_sensor_API_BSEC_configurations:

External sensors API BSEC configurations
========================================

.. _CONFIG_BME68X_IAQ:

CONFIG_BME68X_IAQ
   This option configures the Bosch BSEC library for the BME680.

.. _CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_ULTRA_LOW_POWER:

CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_ULTRA_LOW_POWER
   This option configures the BSEC ultra Low Power Mode. In this mode, the BME680 is sampled every 300 seconds.

.. _CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_LOW_POWER:

CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_LOW_POWER
   This option configures BSEC Low Power Mode. In this mode, the BME680 is sampled every 3 seconds.

.. _CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_CONTINUOUS:

CONFIG_EXTERNAL_SENSORS_BSEC_SAMPLE_MODE_CONTINUOUS
  This option configures BSEC continuous Mode. In this mode, the BME680 is sampled every second.

.. _CONFIG_EXTERNAL_SENSORS_BSEC_TEMPERATURE_OFFSET:

CONFIG_EXTERNAL_SENSORS_BSEC_TEMPERATURE_OFFSET
   This option configures BSEC temperature offset in degree Celsius multiplied by 100.

Module states
*************

The sensor module has an internal state machine with the following states:

* ``STATE_INIT`` - The initial state of the module in which it awaits its initial configuration from the data module.
* ``STATE_RUNNING`` - The module is initialized and can be queried for sensor data.
  It will also send :c:enum:`SENSOR_EVT_MOVEMENT_DATA_READY` on movement.
* ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module.

State transitions take place based on events from other modules, such as the app module, data module, and utility module.

Module events
*************

The :file:`asset_tracker_v2/src/events/sensor_module_event.h` header file contains a list of various events sent by the module.

Dependencies
************

This module uses the following Zephyr API:

* :ref:`Generic sensor API <sensor>`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/sensor_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/sensor_module_event.c`
                :file:`asset_tracker_v2/src/modules/sensor_module.c`

.. doxygengroup:: sensor_module_event
