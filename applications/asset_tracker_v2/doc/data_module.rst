.. _asset_tracker_v2_data_module:

Data module
###########

.. contents::
   :local:
   :depth: 2

The data module gathers data that has been sampled by other modules in the system and stores it into ring buffers.
It keeps track of data requested by the :ref:`asset_tracker_v2_app_module` and decides when data is sent to the cloud.

.. note::
   The data module will undergo substantial refactoring soon. Hence, some of its features are not currently documented.

Features
********

This section documents the features implemented by the module.

Requested data types
====================

The data module maintains a list of requested data types that is updated every time a sample request of type :c:enum:`APP_EVT_DATA_GET` is sent from the :ref:`asset_tracker_v2_app_module`.
When all the data types present in the list is received by the module, the event :c:enum:`DATA_EVT_DATA_READY` is sent out.
This event signifies that all data has been gathered for a sample request.
The available data can now be encoded and sent to the cloud.
The :c:enum:`APP_EVT_DATA_GET` event also carries a sample timeout.
This timeout is used by the data module to trigger a :c:enum:`DATA_EVT_DATA_READY` event, even if not all types in the requested list have been received.
This acts as a fail-safe in case some modules do not manage to sample the requested data in time.
For example, it is not always possible to acquire a GNSS fix.

.. note::
   The module can decide not to send data after a sample request.
   If this happens, data is persisted in the ring buffers and sent to the cloud in batch messages after the next sample request, in case the application is connected to the cloud.
   The ring buffers in the module are implemented so that the oldest entry is always overwritten in case the buffer is filled.

Device configuration
====================

When the application boots, the data module distributes the current values of the applications :ref:`Real-time configurations <real_time_configs>` to the rest of the application with the :c:enum:`DATA_EVT_CONFIG_INIT` event.
These value are persisted in flash between reboots using the :ref:`settings_api` and updated each time the application receives a new configuration update.
If a new configuration update is received from the cloud, the data module distributes the new configuration values using the :c:enum:`DATA_EVT_CONFIG_READY` event.
You can alter the default values of the :ref:`Real-time configurations <real_time_configs>` compile time using the options listed in the :ref:`Default device configuration options <default_config_values>`.

.. _default_config_values:

Configuration options
*********************

.. _CONFIG_DATA_DEVICE_MODE:

CONFIG_DATA_DEVICE_MODE
   This configuration sets the device mode.

.. _CONFIG_DATA_ACTIVE_TIMEOUT_SECONDS:

CONFIG_DATA_ACTIVE_TIMEOUT_SECONDS
   This configuration sets the active mode timeout value.

.. _CONFIG_DATA_MOVEMENT_RESOLUTION_SECONDS:

CONFIG_DATA_MOVEMENT_RESOLUTION_SECONDS
   This configuration sets the movement resolution timeout value.

.. _CONFIG_DATA_MOVEMENT_TIMEOUT_SECONDS:

CONFIG_DATA_MOVEMENT_TIMEOUT_SECONDS
   This configuration sets the movement timeout value.

.. _CONFIG_DATA_ACCELEROMETER_THRESHOLD:

CONFIG_DATA_ACCELEROMETER_THRESHOLD
   This configuration sets the accelerometer threshold value.

.. _CONFIG_DATA_GNSS_TIMEOUT_SECONDS:

CONFIG_DATA_GNSS_TIMEOUT_SECONDS
   This configuration sets the GNSS timeout value.

Module states
*************

The data module has an internal state machine with the following states:

* ``STATE_CLOUD_DISCONNECTED`` - Cloud is disconnected. Data transmission is not attempted.
* ``STATE_CLOUD_CONNECTED`` - Cloud is connected. Data transmission is attempted.
* ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module.

Module events
*************

The :file:`asset_tracker_v2/src/events/data_module_event.h` header file contains a list of events sent by the module.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`settings_api`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/data_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/data_module_event.c`, :file:`asset_tracker_v2/src/modules/data_module.c`

.. doxygengroup:: data_module_event
   :project: nrf
   :members:
