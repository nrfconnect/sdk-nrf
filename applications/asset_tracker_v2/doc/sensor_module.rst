.. _asset_tracker_v2_sensor_module:

Sensor module
#############

.. contents::
   :local:
   :depth: 2

The sensor module interacts with external sensors present on the `Thingy:91 <Thingy:91 product page>`_.
It collects environmental data and detects motion over a set threshold value.

Features
********

This section documents the various features implemented by the module.

Sensor types
============

The following table lists the sensors and sensor data types supported by the module:

+----------------------+-----------------+
| Sensor data type     | External device |
+======================+=================+
| Humidity             | `BME680`_       |
+----------------------+-----------------+
| Temperature          | `BME680`_       |
+----------------------+-----------------+
| Atmospheric Pressure | `BME680`_       |
+----------------------+-----------------+
| Acceleration         | `ADXL362`_      |
+----------------------+-----------------+

The module controls and collects data from the sensors by interacting with their :ref:`device drivers <device_model_api>` using :ref:`Zephyr's generic sensor API <sensor_api>`.

Data sampling
=============

When the module receives an :c:enum:`APP_EVT_DATA_GET` event and the :c:enum:`APP_DATA_ENVIRONMENTAL` type is present in the ``app_data`` list carried in the event, it will sample data.
When data sampling has been carried out, the :c:enum:`SENSOR_EVT_ENVIRONMENTAL_DATA_READY` event is sent from the module with the sampled environmental sensor values.

.. note::
   The nRF9160 DK does not have any external sensors.
   If the sensor module is queried for sensor data when building for the DK, the event :c:enum:`SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED` is sent out by the module
   upon data sampling.

Motion detection
================

Motion is detected when acceleration in either X, Y or Z plane exceeds the configured threshold value.
The threshold is set in one of the following two ways:

* When receiving the :c:enum:`DATA_EVT_CONFIG_INIT` event after boot.
  This event contains the default threshold value set by ``CONFIG_DATA_ACCELEROMETER_THRESHOLD`` or retrieved from flash.
* When receiving the :c:enum:`DATA_EVT_CONFIG_READY` event.
  This occurs when a new threshold value has been updated from cloud.

Both events contain an accelerometer threshold value ``accelerometer_threshold`` in m/s2, present in the event structure.

Motion detection in the module is enabled and disabled in turn to control the number of :c:enum:`SENSOR_EVT_MOVEMENT_DATA_READY` events that are sent out by the sensor module.
This functionality acts as flow control.
It prevents the sensor module from sending events to the rest of the application if the device is accelerating over the threshold for extended periods of time.

The applicaton module controls this behavior through the :c:enum:`APP_EVT_ACTIVITY_DETECTION_ENABLE` and :c:enum:`APP_EVT_ACTIVITY_DETECTION_DISABLE` events.
The sensor module will only send out a :c:enum:`SENSOR_EVT_MOVEMENT_DATA_READY` event if it detects movement and activity detection is enabled.

.. note::
   The DK does not have an external accelerometer.
   However, you can use **Button 2** on the DK to trigger movement for testing purposes.

.. note::
   The accelerometer available on the Thingy:91 needs detailed tuning for each use case to determine reliably which readings are considered as motion.
   This is beyond the scope of the general asset tracker framework this application provides.
   Therefore, the readings are not transmitted to the cloud and are only used to detect a binary active and inactive state.

Configuration options
*********************

.. _CONFIG_SENSOR_THREAD_STACK_SIZE:

CONFIG_SENSOR_THREAD_STACK_SIZE - Sensor module thread stack size
   This option configures the sensor module's internal thread stack size.

Module states
*************

The sensor module has an internal state machine with the following states:

* ``STATE_INIT`` - The initial state of the module in which it awaits its initial configuation from the data module.
* ``STATE_RUNNING`` - The module is initialized and can be queried for sensor data. It will also send :c:enum:`SENSOR_EVT_MOVEMENT_DATA_READY` on movement.
* ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module.

State transitions take place based on events from other modules, such as the app module, data module, and utility module.

Module events
*************

The :file:`asset_tracker_v2/src/events/sensor_module_event.h` header file contains a list of various events sent by the module.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`Generic sensor API <sensor_api>`
* :ref:`adxl362`
* :ref:`bme680`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/sensor_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/sensor_module_event.c`
                :file:`asset_tracker_v2/src/modules/sensor_module.c`

.. doxygengroup:: sensor_module_event
   :project: nrf
   :members:
