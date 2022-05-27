.. _nrf_cloud_mqtt_multi_service:

nRF9160: nRF Cloud MQTT multi-service
#####################################

.. contents::
   :local:
   :depth: 2

This sample is a minimal, error tolerant, integrated demonstration of the :ref:`nrf_cloud <lib_nrf_cloud>`, :ref:`location <lib_location>`, and :ref:`AT Host <lib_at_host>` libraries.
It demonstrates how you can use these libraries together to support Firmware-Over-The-Air (FOTA), location services, modem AT commands over UART, and periodic sensor samples in your `nRF Cloud`_-enabled application.
It also demonstrates how to implement error tolerance in your cellular applications without relying on reboot loops.

.. _nrf_cloud_mqtt_multi_service_requirements:

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm_spm_thingy91.txt

.. _nrf_cloud_mqtt_multi_service_features:

Features
********

This sample implements or demonstrates the following features:

* Error-tolerant use of the `nRF Cloud MQTT API`_ using the :ref:`nrf_modem <nrfxlib:nrf_modem>` and :ref:`nrf_cloud <lib_nrf_cloud>` libraries.
* Support for `Firmware-Over-The-Air (FOTA) update service <nRF Cloud Getting Started FOTA documentation_>`_ using the `nRF Cloud`_ portal.
* Support for `modem AT commands <AT Commands Reference Guide_>`_ over UART using the :ref:`lib_at_host` library.
* Periodic cellular and GNSS location tracking using the :ref:`lib_location` library.
* Periodic temperature sensor sampling on your `Nordic Thingy:91`_, or fake temperature  measurements on your `Nordic nRF9160 DK`_.
* Transmission of sensor and GNSS location samples to the nRF Cloud portal as `nRF Cloud device messages <nRF Cloud Device Messages_>`_.
* Construction of valid `nRF Cloud device messages <nRF Cloud Device Messages_>`_ using `cJSON`_.


.. _nrf_cloud_mqtt_multi_service_structure_and_theory_of_operation:

Structure and theory of operation
*********************************

This sample is separated into a number of smaller functional units.
The top level functional unit and entry point for the sample is the :file:`src/main.c` file.
This file starts three threads, each with a distinct function:

* The connection thread (``conn_thread``) runs the :ref:`nrf_cloud_mqtt_multi_service_connection_management_loop`, which maintains a connection to `nRF Cloud`_.
* The application thread (``app_thread``) Runs the :ref:`nrf_cloud_mqtt_multi_service_application_thread_and_main_application_loop`, which may add `device messages <nRF Cloud Device Messages_>`_ to the :ref:`nrf_cloud_mqtt_multi_service_device_message_queue`.
* The message thread (``msg_thread``) processes the :ref:`nrf_cloud_mqtt_multi_service_device_message_queue` whenever there is an active connection.

.. _nrf_cloud_mqtt_multi_service_connection_management_loop:

Connection management loop
==========================

The connection management loop performs the following four primary tasks:

1) Configures and activates the cellular modem.
#) Starts a connection to the LTE network.
#) Maintains a connection to `nRF Cloud`_.
#) Re-establishes or retries all lost or failed connections.

The modem only needs to be set up and connected to LTE once.
If LTE is lost, the modem automatically works to re-establish it.

The connection to nRF Cloud must be maintained manually by the application.
A significant portion of the :file:`src/connection.c` file is dedicated to this task.

This file also maintains a number of `Zephyr Events`_ tracking the state of the LTE and nRF Cloud connections.
The connection management loop relies on these events to control its progression.

The connection management loop must conform to the :ref:`nRF Cloud connection process <lib_nrf_cloud_connect>`.
This is mostly handled by the :ref:`lib_nrf_cloud` library, with exception to the following behavior:
When a device is first being associated with an nRF Cloud account, the `nRF Cloud MQTT API`_ will send a user association request notification to the device.
Upon receiving this notification, the device must wait for a user association success notification, and then manually disconnect from and reconnect to nRF Cloud.
Notifications of user association success are sent for every subsequent connection after this as well, so the device must only disconnect and reconnect if it previously received a user association request notification.
This behavior is handled by the ``NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST`` and ``NRF_CLOUD_EVT_USER_ASSOCIATED`` cases inside the :c:func:`cloud_event_handler` function in the :file:`src/connection.c` file.

The application must also manually reset its connection to nRF Cloud whenever LTE is lost.
Otherwise, the :ref:`lib_nrf_cloud` library will deadlock.
The ``LTE_LC_EVT_NW_REG_STATUS`` case inside the :c:func:`lte_event_handler` function performs this task.

Upon startup, the connection management loop also updates the `Device Shadow <nRF Cloud Device Shadows_>`_.
This is performed in the :c:func:`update_shadow` function of the :file:`src/connection.c` file.

.. _nrf_cloud_mqtt_multi_service_device_message_queue:

Device message queue
====================

Any thread may submit `device messages <nRF Cloud Device Messages_>`_ to the device message queue, where they are stored until a working connection to `nRF Cloud`_ is established.
Once this happens, the message thread transmits all enqueued device messages, one at a time and in fast succession, to nRF Cloud.
If an enqueued message fails to send, it will be sent back to the queue and tried again later.
If more than :ref:`CONFIG_MAX_CONSECUTIVE_SEND_FAILURES <CONFIG_MAX_CONSECUTIVE_SEND_FAILURES>` messages fail to send in a row, the connection to nRF Cloud is reset and re-established after a short delay.
The transmission pauses again whenever connection to nRF Cloud is lost.
Management of the device message queue is implemented entirely in the :file:`src/connection.c` file.

.. _nrf_cloud_mqtt_multi_service_application_thread_and_main_application_loop:

Application thread and main application loop
============================================

The application thread is implemented in the :file:`src/application.c` file, and is responsible for the high-level behavior of this sample.
It performs the following major tasks:

* Establishes periodic position tracking (which the :ref:`lib_location` library performs).
* Periodically samples temperature data (using the :file:`src/temperature.c` file).
* Constructs timestamped sensor sample and location `device messages <nRF Cloud Device Messages_>`_ using `cJSON`_.
* Sends sensor sample and location device messages to the :ref:`nrf_cloud_mqtt_multi_service_device_message_queue`.

.. note::
   Periodic location tracking is handled by the :ref:`lib_location` library once it has been requested, whereas temperature samples are individually requested by the Main Application Loop.

.. _nrf_cloud_mqtt_multi_service_fota:

FOTA
====

The `FOTA update <nRF Cloud Getting Started FOTA Documentation_>`_ support is almost entirely implemented by enabling the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` option (which is implicitly enabled by :kconfig:option:`CONFIG_NRF_CLOUD_MQTT`).

However, even with :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` enabled, applications must still reboot themselves manually after FOTA download completion, and must still update their `Device Shadow <nRF Cloud Device Shadows_>`_ to reflect FOTA support.

Reboot after download completion is handled by the :file:`src/fota_support.c` file, triggered by a call from the :file:`src/connection.c` file.

`Device Shadow <nRF Cloud Device Shadows_>`_ updates are performed in the :file:`src/connection.c` file.

In a real-world setting, these two behaviors could be directly implemented in the :file:`src/connection.c` file.
In this sample, they are separated for clarity.

.. _nrf_cloud_mqtt_multi_service_temperature_sensing:

Temperature sensing
===================

Temperature sensing is mostly implemented in the :file:`src/temperature.c` file.
This includes generation of false temperature readings on the `Nordic nRF9160 DK`_, which does not have a built-in temperature sensor.

Using the built-in temperature sensor of the `Nordic Thingy:91`_ requires a `devicetree overlay <Zephyr Devicetree Overlays_>`_ file, namely the :file:`boards/thingy91_nrf9160_ns.overlay` file, as well as enabling the Kconfig options :kconfig:option:`CONFIG_SENSOR` and :kconfig:option:`CONFIG_BME680`.
The devicetree overlay file is automatically applied during compilation whenever the ``thingy91_nrf9160_ns`` target is selected.
The required Kconfig options are implicitly enabled by :ref:`CONFIG_TEMP_DATA_USE_SENSOR <CONFIG_TEMP_DATA_USE_SENSOR>`.

.. note::
  For temperature readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_.
  This is performed by the :file:`src/connection.c` file.

.. _nrf_cloud_mqtt_multi_service_location_tracking:

Location tracking
=================

All matters concerning location tracking are handled in the :file:`src/location_tracking.c` file.
For the most part, this amounts to setting up a periodic location request, and then passing the results to a callback configured by the :file:`src/application.c` file.
Additionally, the following two tasks are performed:

* The :ref:`lib_location` library requires GPS assistance data (either A-GPS, P-GPS, or both) to have the fastest possible `time-to-first-fix <TTFF_>`_.
  The library requests this data automatically, but has no way to automatically receive the response.
  Therefore, all MQTT messages received from `nRF Cloud`_ must be be passed directly through to the :ref:`lib_nrf_cloud_agps` and :ref:`lib_nrf_cloud_pgps` libraries to be checked for A-GPS and P-GPS data, respectively.
  The :c:func:`location_assistance_data_handler` function handles this task, and MQTT messages are sent directly to it from the :file:`src/connection.c` file.
* The GNSS module cannot obtain a fix without the antenna first being properly configured.
  This configuration is performed by the :c:func:`gnss_antenna_configure` function.

.. note::
  For location readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_.
  This is performed by the :file:`src/connection.c` file.

.. _nrf_cloud_mqtt_multi_service_test_counter:

Test counter
============

You can enable a test counter by enabling the :ref:`CONFIG_TEST_COUNTER <CONFIG_TEST_COUNTER>` option.
Every time a sensor sample is sent, this counter is incremented by one, and its current value is sent to `nRF Cloud`_.
A plot of the value of the counter over time is automatically shown in the nRF Cloud portal.
This plot is useful for tracking, visualizing, and debugging connection loss, resets, and re-establishment behavior.


.. _nrf_cloud_mqtt_multi_service_device_message_formatting:

Device message formatting
=========================

This sample constructs JSON-based `device messages <nRF Cloud Device Messages_>`_ using `cJSON`_.

While any valid JSON string can be sent as a device message, and accepted and stored by `nRF Cloud`_, there are some pre-designed message structures, known as schemas.
The nRF Cloud portal knows how to interpret these schemas.
These schemas are described in `nRF Cloud application protocols for long-range devices <nRF Cloud JSON protocol schemas_>`_.
The device messages constructed in the :file:`src/application.c` file all adhere to the general message schema.
GNSS and temperature data device messages conform to the ``gps`` and ``temp`` ``deviceToCloud`` schemas respectively.

.. _nrf_cloud_mqtt_multi_service_configuration:

Configuration
*************
|config|

Disabling key features
======================

The following key features of this sample may be independently disabled:

* GNSS-based location tracking - by setting the :ref:`CONFIG_LOCATION_TRACKING_GNSS <CONFIG_LOCATION_TRACKING_GNSS>` option to disabled.
* Cellular-based location tracking - by setting the :ref:`CONFIG_LOCATION_TRACKING_CELLULAR <CONFIG_LOCATION_TRACKING_CELLULAR>` option to disabled.
* Temperature tracking - by setting the :ref:`CONFIG_TEMP_TRACKING <CONFIG_TEMP_TRACKING>` option to disabled.
* GNSS assistance (A-GPS) - by setting the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS` option to disabled.
* Predictive GNSS assistance (P-GPS) - by setting the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS` option to disabled.

If you disable both GNSS and cellular-based location tracking, location tracking is completely disabled.

.. note::
  MQTT should only be used with applications that need to stay connected constantly or transfer data frequently.
  While this sample does allow its core features to be slowed or completely disabled, in real-world applications, you should carefully consider your data throughput and whether MQTT is an appropriate solution.
  If you want to disable or excessively slow all of these features for a real-world application, other solutions, such as the `nRF Cloud Rest API`_, may be more appropriate.

Useful debugging options
========================

To see all debug output for this sample, enable the :ref:`CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL_DBG <CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL_DBG>` option.

To monitor the GNSS module (for instance, to see whether A-GPS or P-GPS assistance data is being consumed), enable the :kconfig:option:`CONFIG_NRF_CLOUD_GPS_LOG_LEVEL_DBG` option.

See also the :ref:`nrf_cloud_mqtt_multi_service_test_counter`.

Configuration options
=====================

Set the following configuration options for the sample:

.. _CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL_DBG:

CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL_DBG - Sample debug logging
   Sets the log level for this sample to debug.

.. _CONFIG_POWER_SAVING_MODE_ENABLE:

CONFIG_POWER_SAVING_MODE_ENABLE - Enable Power Saving Mode (PSM)
   Requests Power Saving Mode from cellular network when enabled.

.. _CONFIG_LTE_INIT_RETRY_TIMEOUT_SECONDS:

CONFIG_LTE_INIT_RETRY_TIMEOUT_SECONDS - LTE initialization retry timeout
   Sets the number of seconds between each LTE modem initialization retry.

.. _CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS:

CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS - Cloud connection retry timeout (seconds)
   Sets the cloud connection retry timeout in seconds.

.. _CONFIG_CLOUD_CONNECTION_REESTABLISH_DELAY_SECONDS:

CONFIG_CLOUD_CONNECTION_REESTABLISH_DELAY_SECONDS - Cloud connection re-establishment delay (seconds)
   Sets the connection re-establishment delay in seconds.
   When the connection to nRF Cloud has been reset, wait for this amount of time before a new attempt to connect.

.. _CONFIG_CLOUD_READY_TIMEOUT_SECONDS:

CONFIG_CLOUD_READY_TIMEOUT_SECONDS - Cloud readiness timeout (seconds)
   Sets the cloud readiness timeout in seconds.
   If the connection to nRF Cloud does not become ready within this timeout, the sample will reset its connection and try again.

.. _CONFIG_DATE_TIME_ESTABLISHMENT_TIMEOUT_SECONDS:

CONFIG_DATE_TIME_ESTABLISHMENT_TIMEOUT_SECONDS - Modem date and time establishment timeout (seconds)
   Sets the timeout for modem date and time establishment (in seconds).
   The sample waits for this number of seconds for the modem to determine the current date and time before giving up and moving on.

.. _CONFIG_APPLICATION_THREAD_STACK_SIZE:

CONFIG_APPLICATION_THREAD_STACK_SIZE - Application Thread Stack Size (bytes)
   Sets the stack size (in bytes) for the application thread of the sample.

.. _CONFIG_CONNECTION_THREAD_STACK_SIZE:

CONFIG_CONNECTION_THREAD_STACK_SIZE - Connection Thread Stack Size (bytes)
   Sets the stack size (in bytes) for the connection thread of the sample.

.. _CONFIG_MESSAGE_THREAD_STACK_SIZE:

CONFIG_MESSAGE_THREAD_STACK_SIZE - Message Queue Thread Stack Size (bytes)
   Sets the stack size (in bytes) for the message queue processing thread of the sample.

.. _CONFIG_MAX_OUTGOING_MESSAGES:

CONFIG_MAX_OUTGOING_MESSAGES - Outgoing message maximum
   Sets the maximum number of messages that can be in the outgoing message queue.
   Messages submitted past this limit will not be enqueued.

.. _CONFIG_MAX_CONSECUTIVE_SEND_FAILURES:

CONFIG_MAX_CONSECUTIVE_SEND_FAILURES - Max outgoing consecutive send failures
   Sets the maximum number of device messages which may fail to send before a connection reset and cooldown is triggered.

.. _CONFIG_CONSECUTIVE_SEND_FAILURE_COOLDOWN_SECONDS:

CONFIG_CONSECUTIVE_SEND_FAILURE_COOLDOWN_SECONDS - Cooldown after max consecutive send failures exceeded (seconds)
   Sets the cooldown time (in seconds) after the maximum number of consecutive send failures is exceeded.
   If a connection reset is triggered by too many failed device messages, the sample waits for this long (in seconds) before trying again.

.. _CONFIG_SENSOR_SAMPLE_INTERVAL_SECONDS:

CONFIG_SENSOR_SAMPLE_INTERVAL_SECONDS - Sensor sampling interval (seconds)
   Sets the time to wait between each temperature sensor sample.

.. note::
   Decreasing the sensor sampling interval too much leads to more frequent use of the LTE connection, which can prevent GNSS from obtaining a fix.
   This is because GNSS can operate only when the LTE connection is not active.
   Every time a sensor sample is sent, it interrupts any attempted GNSS fix.
   The exact time required to obtain a GNSS fix will vary depending on satellite visibility, time since last fix, the type of antenna used, and other environmental factors.
   In good conditions, and with A-GPS data, one minute is a reasonable interval for reliably getting a location estimate.
   This allows using long enough value for :ref:`CONFIG_GNSS_FIX_TIMEOUT_SECONDS <CONFIG_GNSS_FIX_TIMEOUT_SECONDS>`, while still leaving enough time for falling back to cellular positioning if needed.

   The default sensor sampling interval, 60 seconds, will quickly consume cellular data, and should not be used in a production environment.
   Instead, consider using a less frequent interval, and if necessary, combining multiple sensor samples into a single `device message <nRF Cloud Device Messages_>`_ , or combining multiple device messages using the `d2c/bulk device message topic <nRF Cloud MQTT Topics_>`_.

   If you significantly increase the sampling interval, you must keep the :kconfig:option:`CONFIG_MQTT_KEEPALIVE` short to avoid the carrier silently closing the MQTT connection due to inactivity, which could result in dropped device messages.
   The exact maximum value depends on your carrier.
   In general, a few minutes or less should work well.

.. _CONFIG_GNSS_ANTENNA_ONBOARD:

CONFIG_GNSS_ANTENNA_ONBOARD - Select the onboard GNSS antenna (default)
   This option enables the onboard GNSS antenna.

.. _CONFIG_GNSS_ANTENNA_EXTERNAL:

CONFIG_GNSS_ANTENNA_EXTERNAL - Select an external GNSS antenna
   This option enables the external GNSS antenna.

.. _CONFIG_GNSS_AT_MAGPIO:

CONFIG_GNSS_AT_MAGPIO - ``AT%XMAGPIO`` command
   Sets the ``AT%XMAGPIO`` command to be sent to the GNSS module.
   This command must be sent for GNSS to function properly.

   A reasonable value is automatically selected for this option based on the build target.
   You should never have to mess with it yourself unless you are using a non-standard board or custom GNSS antenna.
   See `nRF9160 SiP pin configuration`_ for more details.

.. _CONFIG_GNSS_AT_COEX0:

CONFIG_GNSS_AT_COEX0 - ``AT%XCOEX0`` command
   Sets the ``AT%XCOEX0`` command to be sent to the GNSS module.
   This command must be sent for GNSS to function properly.

   A reasonable value is automatically selected for this option based on the build target and selected antenna configuration.
   You should never have to mess with it yourself unless you are using a non-standard board or custom GNSS antenna.
   See `nRF9160 SiP pin configuration`_ for more details.

.. _CONFIG_GNSS_FIX_TIMEOUT_SECONDS:

CONFIG_GNSS_FIX_TIMEOUT_SECONDS - GNSS fix timeout (seconds)
   Sets the GNSS fix timeout in seconds.
   On each location sample, try for this long to achieve a GNSS fix before falling back to cellular positioning.

.. _CONFIG_LOCATION_TRACKING_SAMPLE_INTERVAL_SECONDS:

CONFIG_LOCATION_TRACKING_SAMPLE_INTERVAL_SECONDS - Location sampling interval (seconds)
   Sets the location sampling interval in seconds.

.. _CONFIG_LOCATION_TRACKING_GNSS:

CONFIG_LOCATION_TRACKING_GNSS - GNSS location tracking
   Enables GNSS location tracking.
   Disable both this and :ref:`CONFIG_LOCATION_TRACKING_CELLULAR <CONFIG_LOCATION_TRACKING_CELLULAR>` to completely disable location tracking.
   Defaults to enabled.

.. _CONFIG_LOCATION_TRACKING_CELLULAR:

CONFIG_LOCATION_TRACKING_CELLULAR - Cellular location tracking
   Enables cellular location tracking.
   Disable both this and :ref:`CONFIG_LOCATION_TRACKING_GNSS <CONFIG_LOCATION_TRACKING_GNSS>` to completely disable location tracking.
   Defaults to enabled.

.. _CONFIG_TEMP_DATA_USE_SENSOR:

CONFIG_TEMP_DATA_USE_SENSOR - Use genuine temperature data
   Sets whether to take genuine temperature measurements from a connected BME680 sensor, or just simulate sensor data.

.. _CONFIG_TEMP_TRACKING:

CONFIG_TEMP_TRACKING - Track temperature data
   Enables tracking and reporting of temperature data to `nRF Cloud`_.
   Defaults to enabled.

.. _CONFIG_TEST_COUNTER:

CONFIG_TEST_COUNTER - Enable test counter
   Enables the test counter.

.. _nrf_cloud_mqtt_multi_service_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/nrf_cloud_mqtt_multi_service`

.. include:: /includes/thingy91_build_and_run.txt

.. _nrf_cloud_mqtt_multi_service_dependencies:

Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_nrf_cloud`
* :ref:`lib_location`
* :ref:`lib_at_host`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware components:

* :ref:`secure_partition_manager`
* :ref:`Trusted Firmware-M <ug_tfm>`
