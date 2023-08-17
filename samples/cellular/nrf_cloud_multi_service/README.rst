.. _nrf_cloud_multi_service:

Cellular: nRF Cloud multi-service
#################################

.. contents::
   :local:
   :depth: 2

This sample is a minimal, error tolerant, integrated demonstration of the :ref:`lib_nrf_cloud`, :ref:`lib_location`, and :ref:`lib_at_host` libraries.
It demonstrates how you can integrate Firmware-Over-The-Air (FOTA), Location Services, Alert and Log Services, periodic sensor sampling, and more in your `nRF Cloud`_-enabled application.
It also demonstrates how to build connected, error-tolerant applications without worrying about physical-level specifics using Zephyr's ``conn_mgr``.

.. _nrf_cloud_multi_service_requirements:

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. include:: /includes/external_flash_nrf91.txt

For the nRF9160 DK, when using `CoAP`_, use modem firmware version 1.3.5 or above to benefit from the power savings provided by `DTLS Connection ID <RFC 9146 - Connection Identifier for DTLS 1.2_>`_.

.. _nrf_cloud_multi_service_features:

Features
********

This sample implements or demonstrates the following features:

* Generic, disconnect-tolerant integration of LTE using Zephyr's ``conn_mgr`` and :kconfig:option:`CONFIG_LTE_CONNECTIVITY`.
* Error-tolerant use of the nRF Cloud CoAP API using the :ref:`lib_nrf_cloud_coap` CoAP library.
* Error-tolerant use of the `nRF Cloud MQTT API`_ using the :ref:`lib_nrf_cloud` library.
* Support for `Firmware-Over-The-Air (FOTA) update service <nRF Cloud Getting Started FOTA documentation_>`_ using the `nRF Cloud`_ portal.
* Support for `modem AT commands <AT Commands Reference Guide_>`_ over UART using the :ref:`lib_at_host` library.
* Support for remote execution of modem AT commands using application-specific device messages.
* Periodic cellular, Wi-Fi, and GNSS location tracking using the :ref:`lib_location` library.
* Periodic temperature sensor sampling on your `Nordic Thingy:91`_, or fake temperature  measurements on your `Nordic nRF9161 DK`_, or `Nordic nRF9160 DK`_.
* Transmission of sensor and GNSS location samples to the nRF Cloud portal as `nRF Cloud device messages <nRF Cloud Device Messages_>`_.
* Construction of valid `nRF Cloud device messages <nRF Cloud Device Messages_>`_.
* Minimal LED status indication using the `Zephyr LED API`_.
* Transmission of an alert on sample startup using the :ref:`lib_nrf_cloud_alert` library.
* Transmission of additional alerts, whenever a specified temperature limit is exceeded.
* Optional transmission of log messages to the cloud using the :ref:`lib_nrf_cloud_log` library.
* Experimental support for Wi-Fi connectivity.

.. _nrf_cloud_multi_service_structure_and_theory_of_operation:

Structure and theory of operation
*********************************

This sample is separated into a number of smaller functional units.
The top level functional unit and entry point for the sample is the :file:`src/main.c` file.
This file starts three primary threads, each with a distinct function:

* The cloud connection thread (``con_thread``, :file:`src/cloud_connection.c`) runs the :ref:`nrf_cloud_multi_service_cloud_connection_loop`, which maintains a connection to `nRF Cloud`_.
* The application thread (``app_thread``, :file:`src/application.c`) runs the :ref:`nrf_cloud_multi_service_application_thread_and_main_application_loop`, which controls demo features and submits `device messages <nRF Cloud Device Messages_>`_ to the :ref:`nrf_cloud_multi_service_device_message_queue`.
* The message queue thread (``msg_thread``, :file:`src/message_queue.c`) then transmits these messages whenever there is an active connection.
  See :ref:`nrf_cloud_multi_service_device_message_queue`.

:file:`src/main.c` also optionally starts a fourth thread, the ``led_thread``, which animates any onboard LEDs if :ref:`nrf_cloud_multi_service_led_status_indication` is enabled.

When using CoAP, two additional threads start:

* The CoAP FOTA job checking thread (``coap_fota``, :file:`src/fota_support_coap.c`) runs the :ref:`nrf_cloud_multi_service_coap_fota_loop` which periodically asks nRF Cloud for any pending FOTA job.
* The CoAP shadow delta checking thread (``coap_shadow``, :file:`src/shadow_support_coap.c`) runs the :ref:`nrf_cloud_multi_service_coap_shadow_loop` which periodically asks nRF Cloud for any shadow changes.

.. _nrf_cloud_multi_service_cloud_connection_loop:

Cloud connection loop
=====================

The cloud connection loop (implemented in :file:`src/cloud_connection.c`) monitors network availability.
It starts a connection with `nRF Cloud`_ whenever the Internet becomes reachable, and closes that connection whenever Internet access is lost.
It has error handling and timeout features to ensure that failed or lost connections are re-established after a waiting period (:ref:`CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS <CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS>`).

Since the :kconfig:option:`CONFIG_LTE_CONNECTIVITY` Kcofig option is enabled, Zephyr's ``conn_mgr`` automatically enables and connects to LTE.

Whenever a connection to nRF Cloud is started, the cloud connection loop follows the :ref:`nRF Cloud connection process <lib_nrf_cloud_connect>`.
The :ref:`lib_nrf_cloud` library handles most of the connection process, with exception to the following behavior:

When an MQTT device is first being associated with an nRF Cloud account, the `nRF Cloud MQTT API`_ will send a user association request notification to the device.
Upon receiving this notification, the device must wait for a user association success notification, and then manually disconnect from and reconnect to nRF Cloud.
Notifications of user association success are sent for every subsequent connection after this as well, so the device must only disconnect and reconnect if it previously received a user association request notification.
This behavior is handled by the ``NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST`` and ``NRF_CLOUD_EVT_USER_ASSOCIATED`` cases inside the :c:func:`cloud_event_handler` function.

When a CoAP device attempts to connect to nRF Cloud, the connection will fail to be authenticated until the device is provisioned to an nRF Cloud account.
See :ref:`nrf_cloud_multi_service_coap_provisioning` for details.

Upon startup, the cloud connection loop also updates the `device shadow <nRF Cloud Device Shadows_>`_.
This is performed in the :c:func:`update_shadow` function.

.. _nrf_cloud_multi_service_device_message_queue:

Device message queue
====================

Any thread may submit `device messages <nRF Cloud Device Messages_>`_ to the device message queue (implemented in :file:`src/message_queue.c`), where they are stored until a working connection to `nRF Cloud`_ is established.
Once this happens, the message thread transmits all enqueued device messages, one at a time and in fast succession, to nRF Cloud.
If an enqueued message fails to send, it will be sent back to the queue and tried again later.
Transmission is paused whenever connection to nRF Cloud is lost.
If more than :ref:`CONFIG_MAX_CONSECUTIVE_SEND_FAILURES <CONFIG_MAX_CONSECUTIVE_SEND_FAILURES>` messages in a row fail to send, the connection to nRF Cloud is reset, and then reconnected after a delay (:ref:`CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS <CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS>`).

Most messages sent to nRF Cloud by this sample are sent using the message queue, but the following are sent directly:

* Alerts using the :ref:`lib_nrf_cloud_alert` library.
* Logs using the :ref:`lib_nrf_cloud_log` library.
* `Device shadow <nRF Cloud Device Shadows_>`_ updates.
* Ground fix requests from the :ref:`lib_location` library.

.. _nrf_cloud_multi_service_application_thread_and_main_application_loop:

Application thread and main application loop
============================================

The application thread is implemented in the :file:`src/application.c` file, and is responsible for the high-level behavior of this sample.
It performs the following major tasks:

* Establishes periodic position tracking (which the :ref:`lib_location` library performs).
* Periodically samples temperature data (using the :file:`src/temperature.c` file).
* Constructs timestamped sensor sample and location `device messages <nRF Cloud Device Messages_>`_.
* Sends sensor sample and location device messages to the :ref:`nrf_cloud_multi_service_device_message_queue`.
* Checks for and executes :ref:`remote modem AT command requests <nrf_cloud_multi_service_remote_at>`.
* Sends temperature alerts and sample startup alerts using the :ref:`lib_nrf_cloud_alert` library.

.. note::
   Periodic location tracking is handled by the :ref:`lib_location` library once it has been requested, whereas temperature samples are individually requested by the Main Application Loop.

.. _nrf_cloud_multi_service_coap_fota_loop:

CoAP FOTA job check loop
========================

The nRF Cloud CoAP service provides device-initiated communication with the server.
Server to device communication is only in response to a device request, which means the device polls for asynchronous changes to server resources.
The CoAP FOTA job checking thread performs the following tasks:

* Checks the settings storage area to see if the device has recently rebooted after performing a FOTA update.
* If it had recently rebooted, it sends the results of the FOTA update to the server with the :c:func:`nrf_cloud_coap_fota_job_update` function.
* Checks for a new FOTA job with the :c:func:`nrf_cloud_coap_fota_job_get` function.
* If one is available, it downloads the update using HTTPS, saves the job information in settings, then reboots the device.
* If the download was unsuccessful, it notifies the server with the :c:func:`nrf_cloud_coap_fota_job_update` function.

.. _nrf_cloud_multi_service_coap_shadow_loop:

CoAP shadow delta checking loop
===============================

The CoAP shadow delta checking thread performs the following tasks:

* Checks for a change in the device shadow with the :c:func:`nrf_cloud_coap_shadow_get` function.
* Parse and process the JSON shadow delta document.
* If a change is received, the thread sends the change back with the :c:func:`nrf_cloud_coap_shadow_state_update` function.
  This is necessary to prevent the device from unnecessarily receiving the same shadow delta the next time through the loop.

.. _nrf_cloud_multi_service_fota:

FOTA
====

When using MQTT, the `FOTA update <nRF Cloud Getting Started FOTA Documentation_>`_ support is almost entirely implemented by enabling the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` option (which is implicitly enabled by :kconfig:option:`CONFIG_NRF_CLOUD_MQTT`).

However, even with :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` enabled, applications must still reboot themselves manually after FOTA download completion, and must still update their `Device Shadow <nRF Cloud Device Shadows_>`_ to reflect FOTA support.

Reboot after download completion is handled by the :file:`src/fota_support.c` file, triggered by a call from the :file:`src/cloud_connection.c` file.

`Device Shadow <nRF Cloud Device Shadows_>`_ updates are performed in the :file:`src/cloud_connection.c` file.

In a real-world setting, these two behaviors could be directly implemented in the :file:`src/cloud_connection.c` file.
In this sample, they are separated for clarity.

This sample supports full modem FOTA for the nRF91 Series development kit.
Version 0.14.0 or higher is required for nRF9160 DK.
To enable full modem FOTA, add the following parameter to your build command:

``-DOVERLAY_CONFIG=overlay_full_modem_fota.conf``

Also, specify your development kit version by appending it to the board name.
For example, if you are using an nRF9160 DK version 1.0.1, use the following board name in your build command:

``nrf9160dk_nrf9160_ns@1_0_1``

This sample also supports placement of the MCUboot secondary partition in external flash for the nRF9161 DK, and for nRF9160 DK version 0.14.0 and higher.
To enable this, add the following parameter to your build command:

``-DOVERLAY_CONFIG=overlay_mcuboot_ext_flash.conf``

Then specify your development kit version as described earlier.

.. _nrf_cloud_multi_service_temperature_sensing:

Temperature sensing
===================

Temperature sensing is mostly implemented in the :file:`src/temperature.c` file.
This includes generation of false temperature readings on your nRF91 Series DK, which does not have a built-in temperature sensor.

Using the built-in temperature sensor of the `Nordic Thingy:91`_ requires a `devicetree overlay <Zephyr Devicetree Overlays_>`_ file, namely the :file:`boards/thingy91_nrf9160_ns.overlay` file, as well as enabling the Kconfig options :kconfig:option:`CONFIG_SENSOR` and :kconfig:option:`CONFIG_BME680`.
The devicetree overlay file is automatically applied during compilation whenever the ``thingy91_nrf9160_ns`` target is selected.
The required Kconfig options are implicitly enabled by :ref:`CONFIG_TEMP_DATA_USE_SENSOR <CONFIG_TEMP_DATA_USE_SENSOR>`.

.. note::
  For temperature readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_.
  This is performed by the :file:`src/cloud_connection.c` file.

.. _nrf_cloud_multi_service_location_tracking:

Location tracking
=================

All matters concerning location tracking are handled in the :file:`src/location_tracking.c` file.
This involves setting up a periodic location request, and then passing the results to a callback configured by the :file:`src/application.c` file.

For location readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_.
This is performed by the :file:`src/cloud_connection.c` file.

Each enabled location method is tried in the following order until one succeeds: GNSS, Wi-Fi, then cellular.

The GNSS and cellular location tracking methods are enabled by default and will work on both Thingy:91 and nRF91 Series DK targets.

.. _nrf_cloud_multi_service_wifi_location_tracking:

The Wi-Fi location tracking method is not enabled by default and requires the nRF7002 Wi-Fi companion chip.

When enabled, this location method scans the MAC addresses of nearby access points and submits them to nRF Cloud to obtain a location estimate.

See :ref:`nrf_cloud_multi_service_building_wifi_scan` for details on how to enable Wi-Fi location tracking.

This sample supports placing P-GPS data in external flash for the nRF91 Series development kit.
Version 0.14.0 or later is required for the nRF9160 DK.
To enable this, add the following parameter to your build command:

``-DOVERLAY_CONFIG=overlay_pgps_ext_flash.conf``

Also, specify your development kit version by appending it to the board name.
For example, if you are using an nRF9160 development kit version 1.0.1, use the following board name in your build command:

``nrf9160dk_nrf9160_ns@1_0_1``

.. _nrf_cloud_multi_service_remote_at:

Remote execution of modem AT commands (MQTT only)
=================================================

If the :ref:`CONFIG_AT_CMD_REQUESTS <CONFIG_AT_CMD_REQUESTS>` Kconfig option is enabled, you can remotely execute modem AT commands on your device by sending a device message with appId ``MODEM``, messageType ``CMD``, and the data key set to the command you would like to execute.

The application executes the command stored in the data key, and responds with a device message containing either an error code or the response from the modem to the AT command.

For example, if you send the following device message to a device running this sample with :ref:`CONFIG_AT_CMD_REQUESTS <CONFIG_AT_CMD_REQUESTS>` enabled:

.. code-block:: json

   {"appId":"MODEM", "messageType":"CMD", "data":"AT+CGMR"}

It executes the modem AT command ``AT+CGMR`` and sends a device message similar to the following back to nRF Cloud (an example of nRF9160, modem firmware v1.3.2):

.. code-block:: json

   {
      "appId": "MODEM",
      "messageType": "DATA",
      "ts": 1669244834095,
      "data": "mfw_nrf9160_1.3.2\r\nOK"
   }


To do this in the nRF Cloud portal, write `{"appId":"MODEM", "messageType":"CMD", "data":"AT+CGMR"}` into the **Send a message** box of the **Terminal** card and click :guilabel:`Send`.

.. _nrf_cloud_multi_service_led_status_indication:

LED status indication
=====================

On boards that support LED status indication, this sample can indicate its current status with any on-board LEDs.

This is performed by a background thread implemented in the :file:`src/led_control.c` file.

Other threads may request either a temporary or indefinite LED pattern.
This wakes up the ``led_thread``, which begins animating the requested pattern, sleeping for 100 milliseconds at a time between animation frames, until the requested pattern has completed (if it is temporary), or until a new pattern is requested in its place.

This feature is enabled by default for the ``thingy91_nrf9160_ns`` (Thingy:91), ``nrf9161dk_nrf9161_ns`` (nRF9161 DK), and ``nrf9160dk_nrf9160_ns`` (nRF9160 DK) targets.

The patterns displayed, the states they describe, and the options required for them to appear are as follows:

+------------------------------------------------------+--------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Status                                               | Thingy:91                            | nRF91 Series DK                                                                                        | Conditions                                                                                                |
+======================================================+======================================+========================================================================================================+===========================================================================================================+
| Trying to connect to nRF Cloud (for the first time)  | Pulsating orange LED                 | Single LED lit, spinning around the square of LEDs                                                     | LED status indication is enabled                                                                          |
+------------------------------------------------------+--------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Connection to nRF Cloud lost, reconnecting           | Pulsating orange LED                 | Single LED lit, spinning around the square of LEDs                                                     | The :ref:`CONFIG_LED_VERBOSE_INDICATION <CONFIG_LED_VERBOSE_INDICATION>` option is enabled                |
+------------------------------------------------------+--------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Fatal error                                          | Blinking red LED, 75% duty cycle     | All four LEDs blinking, 75% duty cycle                                                                 | LED status indication is enabled                                                                          |
+------------------------------------------------------+--------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Device message sent successfully                     | Blinking green LED, 25% duty cycle   | Alternating checkerboard pattern (two LEDs are lit at a time, either LED1 and LED4, or LED2 and LED3)  | The :ref:`CONFIG_LED_VERBOSE_INDICATION <CONFIG_LED_VERBOSE_INDICATION>` option is enabled                |
+------------------------------------------------------+--------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Idle                                                 | LED pulsating between blue and cyan  | Single LED lit, bouncing between opposite corners (LED1 and LED4)                                      | The :ref:`CONFIG_LED_CONTINUOUS_INDICATION <CONFIG_LED_CONTINUOUS_INDICATION>` option is enabled          |
+------------------------------------------------------+--------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+

Under all other circumstances, on-board LEDs are turned off.

.. note::
  The :ref:`CONFIG_LED_VERBOSE_INDICATION <CONFIG_LED_VERBOSE_INDICATION>` and :ref:`CONFIG_LED_CONTINUOUS_INDICATION <CONFIG_LED_CONTINUOUS_INDICATION>` options are enabled by default.

See :ref:`nrf_cloud_multi_service_customizing_LED_status_indication` for details on customizing the LED behavior.

See :ref:`nrf_cloud_multi_service_led_third_party` for details on configuring LED status indication on third-party boards.

.. _nrf_cloud_multi_service_test_counter:

Test counter
============

You can enable a test counter by enabling the :ref:`CONFIG_TEST_COUNTER <CONFIG_TEST_COUNTER>` option.
Every time a sensor sample is sent, this counter is incremented by one, and its current value is sent to `nRF Cloud`_.
A plot of the value of the counter over time is automatically shown in the nRF Cloud portal.
This plot is useful for tracking, visualizing, and debugging connection loss, resets, and re-establishment behavior.


.. _nrf_cloud_multi_service_device_message_formatting:

Device message formatting
=========================

This sample, when using MQTT to communicate with nRF Cloud, constructs JSON-based `device messages <nRF Cloud Device Messages_>`_.

While any valid JSON string can be sent as a device message, and accepted and stored by `nRF Cloud`_, there are some pre-designed message structures, known as schemas.
The nRF Cloud portal knows how to interpret these schemas.
These schemas are described in `nRF Cloud application protocols for long-range devices <nRF Cloud JSON protocol schemas_>`_.
The device messages constructed in the :file:`src/application.c` file all adhere to the general message schema.

GNSS and temperature data device messages conform to the ``gnss`` and ``temp`` ``deviceToCloud`` schemas respectively.

This sample, when using CoAP to communicate with nRF Cloud, uses the :ref:`lib_nrf_cloud_coap` library to construct and transmit CBOR-based device messages.
Some CoAP traffic, specifically AWS shadow traffic, remains in JSON format.

.. _nrf_cloud_multi_service_configuration:

Configuration
*************
|config|

Disabling key features
======================

The following key features of this sample may be independently disabled:

* GNSS-based location tracking - by setting the :ref:`CONFIG_LOCATION_TRACKING_GNSS <CONFIG_LOCATION_TRACKING_GNSS>` option to disabled.
* Cellular-based location tracking - by setting the :ref:`CONFIG_LOCATION_TRACKING_CELLULAR <CONFIG_LOCATION_TRACKING_CELLULAR>` option to disabled.
* Wi-Fi-based location tracking - by setting the :ref:`CONFIG_LOCATION_TRACKING_WIFI <CONFIG_LOCATION_TRACKING_CELLULAR>` option to disabled.
* Temperature tracking - by setting the :ref:`CONFIG_TEMP_TRACKING <CONFIG_TEMP_TRACKING>` option to disabled.
* GNSS assistance (A-GPS) - by setting the :kconfig:option:`CONFIG_NRF_CLOUD_AGPS` option to disabled.
* Predictive GNSS assistance (P-GPS) - by setting the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS` option to disabled.
* FOTA when using MQTT - by setting the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` option to disabled.
* FOTA when using CoAP - by setting the :kconfig:option:`NRF_CLOUD_COAP_FOTA` option to disabled.

If you disable GNSS, Wi-Fi-based, and cellular-based location tracking, location tracking is completely disabled.

.. note::
  MQTT should only be used with applications that need to stay connected constantly or transfer data frequently.
  While this sample does allow its core features to be slowed or completely disabled, in real-world applications, you should carefully consider your data throughput and whether MQTT is an appropriate solution.
  If you want to disable or excessively slow all of these features for a real-world application, other solutions, such as the `nRF Cloud Rest API`_, may be more appropriate.

Customizing GNSS antenna configuration
======================================

This sample uses the :ref:`lib_modem_antenna` library, which is enabled by default for builds targeting the ``nrf9161dk_nrf9161_ns``, ``nrf9160dk_nrf9160_ns``, or ``thingy91_nrf9160_ns`` board names.

If you are using a different board or build target, or would like to use a custom or external GNSS antenna, see the :ref:`lib_modem_antenna` library documentation for configuration instructions.

Enable :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` to use an external antenna.

.. _nrf_cloud_multi_service_customizing_LED_status_indication:

Customizing LED status indication
=================================

To disable LED status indication (other than the selected idle behavior) after a connection to nRF Cloud has been established at least once, disable :ref:`CONFIG_LED_VERBOSE_INDICATION <CONFIG_LED_VERBOSE_INDICATION>`.

To turn the LED off while the sample is idle (rather than show an idle pattern), disable :ref:`CONFIG_LED_CONTINUOUS_INDICATION <CONFIG_LED_CONTINUOUS_INDICATION>`.

If you disable both of these options together, the status indicator LED remains off after a connection to nRF Cloud has been established at least once.

.. _nrf_cloud_multi_service_led_third_party:

Configuring LED status indication for third-party boards
========================================================

This sample assumes that the target board either has a single RGB LED with PWM support, or four discrete LEDs available.

For third-party boards, you can select the RGB LED option by enabling both the :ref:`CONFIG_LED_INDICATION_PWM <CONFIG_LED_INDICATION_PWM>` and :ref:`CONFIG_LED_INDICATION_RGB <CONFIG_LED_INDICATOR_RGB>` options.
In this case, the board must have a devicetree entry marked as compatible with the `Zephyr pwm-leds`_ driver.

Otherwise, the four-LED option (:ref:`CONFIG_LED_INDICATION_GPIO <CONFIG_LED_INDICATION_GPIO>` and :ref:`CONFIG_LED_INDICATOR_4LED <CONFIG_LED_INDICATOR_4LED>`) is selected by default as long as there is a devicetree entry compatible with the `Zephyr gpio-leds`_ driver.

The four-LED option should work even if there are not four LEDs available, as long as an appropriate devicetree entry exists.
However, if fewer than four LEDs are available, the patterns may be difficult to identify.

To add your own LED indication implementations, you can add values to the ``LED_INDICATOR`` Kconfig choice and modify the :file:`src/led_control.c` file accordingly.

To disable LED indication, enable the :ref:`CONFIG_LED_INDICATION_DISABLED <CONFIG_LED_INDICATION_DISABLED>` option.

For examples of how to set up devicetree entries compatible with the Zephyr ``gpio-leds`` and ``pwm-leds`` drivers, see the files :file:`zephyr/boards/arm/nrf9160dk_nrf9160/nrf9160dk_nrf9160_common.dts`, :file:`zephyr/boards/arm/nrf9161dk_nrf9161/nrf9161dk_nrf9161_common.dts` and :file:`zephyr/boards/arm/thingy91_nrf9160/thingy91_nrf9160_common.dts`.
Search for nodes with ``compatible = "gpio-leds";`` and ``compatible = "pwm-leds";`` respectively.

Useful debugging options
========================

To see all debug output for this sample, enable the :ref:`CONFIG_MULTI_SERVICE_LOG_LEVEL_DBG <CONFIG_MULTI_SERVICE_LOG_LEVEL_DBG>` option.

To monitor the GNSS module (for instance, to see whether A-GPS or P-GPS assistance data is being consumed), enable the :kconfig:option:`CONFIG_NRF_CLOUD_GPS_LOG_LEVEL_DBG` option.

See also the :ref:`nrf_cloud_multi_service_test_counter`.


.. _nrf_cloud_multi_service_hardcoded_certs:

Provisioning with hard-coded CA and device certificates
=======================================================

The :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option allows you to use hard-coded CA and device certificates stored in unprotected program memory for connecting to nRF Cloud.

.. important::
   The :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option is not secure, and should be used only for demonstration purposes!
   Because this setting stores your device's private key in unprotected program memory, using it makes your device vulnerable to impersonation.

If you are certain you understand the inherent security risks, you can provision your device with :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` as follows:

First, generate a self-managed CA certificate:

1. Download and install the `nRF Cloud Utilities <nRF Cloud Utilities documentation_>`_ repo.
#. Use the nRF Cloud Utilities :file:`create_ca_cert.py` Python script to generate the certificate:

   .. code-block:: console

      python3 create_ca_cert.py -c US -f self_ca_

   Remember to set ``-c`` to your two-letter country code.
   See the `Create CA Cert <nRF Cloud Utilities Create CA Cert_>`_ section in the nRF Cloud Utilities documentation for more details.

   You should now have three files:

     * :file:`self_ca_<certificate_serial>_ca.pem`
     * :file:`self_ca_<certificate_serial>_prv.pem`
     * :file:`self_ca_<certificate_serial>_pub.pem`

   Where ``<certificate_serial>`` is your certificate's serial number in hex.
   You will use these files to sign device certificates.

   You only need to generate these three files once.
   They can be used to sign as many device certificates as you need.

Next, complete the following steps for each device you wish to provision:

1. Create a globally unique `device ID <nRF Cloud Device ID_>`_ for the device.

   The ID can be anything you want, as long as it is not prefixed with ``nrf-`` and is globally unique.

   Each device should have its own device ID, and you must use this device ID exactly in all of the remaining steps for that device, otherwise provisioning will fail.

#. Use the :file:`create_device_credentials.py` Python script to generate a self-signed certificate for the device:

   .. code-block:: console

      python3 create_device_credentials.py -ca "self_ca_<certificate_serial>_ca.pem" -ca_key "self_ca_<certificate_serial>_prv.pem" -c US -cn "<device_uuid>" -f cred_

   Where ``<device_uuid>`` is the device ID you created, and :file:`self_ca_<certificate_serial>_ca.pem`, and :file:`self_ca_<certificate_serial>_prv.pem` are the corresponding file names from the previous step.

   Remember to set ``-c`` to your two-letter country code.
   See the `Create Device Credentials <nRF Cloud Utilities Create Device Credentials_>`_ section in the nRF Cloud Utilities documentation for more details.

   You should now have an additional three files:

     * :file:`cred_<device_uuid>_crt.pem`
     * :file:`cred_<device_uuid>_pub.pem`
     * :file:`cred_<device_uuid>_prv.pem`

   These three files can only be used by a single device.

   .. important::

      If an attacker obtains the private key contained in :file:`cred_<device_uuid>_prv.pem`, they will be able to impersonate your device.
      Since using hard-coded certificates does not protect this private key from extraction, this option should not be used in production!

#. Create a CSV file named :file:`<device_uuid>_cred.csv` and give it the following contents:

   .. code-block:: none

      <device_uuid>,,,,"<device_cert>"

   Where ``<device_uuid>`` is replaced by the device ID you created, and ``<device_cert>`` is replaced by the exact contents of :file:`<device_uuid>_crt.pem`.

   For example, if the device ID you created is ``698d4c11-0ccc-4f04-89cd-6882724e3f6f``:

   .. code-block:: none

      698d4c11-0ccc-4f04-89cd-6882724e3f6f,,,,"-----BEGIN CERTIFICATE-----
      sCC8AtbNQhzbp4y01FEzXaf5Fko3Qdq0o5LbuNpVA7S6AKAkjt17QzKJAiGWHakh
      RnwzoA2dF4wR0rMP5vR6dqBblaGAA5hN7GE2vPBHTDNZGJ6tZ9dnO6446dg9gGds
      eeadE1HdVnUj8nb+7CGm39vJ4fuNk9vogH0nMdxjCnXAinoOMRx8EklQsR747+Gz
      sxcdVYuNEb/E2vWBHTDNZGJ6tZC1JC9d6/RC3Vb1JC4tWnK9mk/Jw984ZuYugpMc
      1t9umoGFYCz0nMdxjCnXAbnoOMC5A0RxcWPzxfC5A0RH+j+mwoMxwhgfFY4EhVxp
      oCC8labNQhzRC3Vc1JC4tWnK9mpVA7k/o5LbuNpVA7S6AKAkjt17QzKJAiGWHakh
      RXwcoAndF4wPzxfC5A0RHponmwBHTDoM7GE2vPBHTDNZGJ6tZ9dnO6446dg9gGds
      eefdE1HcVnULbuNpVA7S6AKAkjxjCnt1gH0nMdxjCnXAinoOMRx8EklQsR747+Fz
      srm/VYaNEb/E2vPBHTDNZGJ6tZc1JC9d6/RC3Vc1JC4tWnK9mk/Jr984ZuYugpMc
      nt9uZTGFYCzZD0FFAA5NAC4i1PARStFycWPzxfC5A0RqodhswoMxwhgfFY4EhVx=
      -----END CERTIFICATE-----
      "

   Do not attempt to use this example directly, it has been filled with dummy data.
   See `nRF Cloud REST API ProvisionDevices`_ for more details.

#. Navigate to the `Provision Devices <nRF Cloud Portal Provision Devices_>`_ page of the nRF Cloud portal and upload the :file:`<device_uuid>_cred.csv` file you created.

   You should see a message stating that the file was uploaded successfully, and a device with the device ID you created should appear in the `Devices <nRF Cloud Portal Devices_>`_ page.

#. Download `Amazon Root CA 1`_ and save it as :file:`AmazonRootCA1.pem`.

   See `CA certificates for server authentication in AWS IoT Core`_ for more details.
   Do not confuse this CA cert with your self-managed CA cert.

#. Prepare the :file:`cred_<device_uuid>_crt.pem`, :file:`cred_<device_uuid>_prv.pem`, and :file:`AmazonRootCA1.pem` files for use by the device.

   Your device needs these files in order to connect to nRF Cloud.
   To be usable by your device, they need to be reformatted as C strings and given specific names (:file:`client-cert.pem`, :file:`private-key.pem` and :file:`ca-cert.pem` respectively).

   You can perform the reformatting and renaming as follows:

   .. tabs::

      .. group-tab:: Bash

         .. code-block:: console

           awk 'NF { print "\""$0"\\n\""}' "cred_<device_uuid>_crt.pem" > client-cert.pem
           awk 'NF { print "\""$0"\\n\""}' "cred_<device_uuid>_prv.pem" > private-key.pem
           awk 'NF { print "\""$0"\\n\""}' AmazonrootCA1.pem > ca-cert.pem

         Where ``<device_uuid>`` is replaced with the device ID you created.

      .. group-tab:: PowerShell

         .. code-block:: console

            gc ".\cred_<device_uuid>_crt.pem" | %{"""$_\n"""} | out-file -encoding utf8 client-cert.pem
            gc ".\cred_<device_uuid>_prv.pem" | %{"""$_\n"""} | out-file -encoding utf8 private-key.pem
            gc .\AmazonrootCA1.pem | %{"""$_\n"""} | out-file -encoding utf8 ca-cert.pem

         Where ``<device_uuid>`` is replaced with the device ID you created.

   You should now have the following three files:

     * :file:`client-cert.pem`
     * :file:`private-key.pem`
     * :file:`ca-cert.pem`

   And the contents of each file should be similar to the following:

   .. code-block:: C

      "-----BEGIN CERTIFICATE-----\n"
      "sCC8AtbNQhzbp4y01FEzXaf5Fko3Qdq0o5LbuNpVA7S6AKAkjt17QzKJAiGWHakh\n"
      "RnwzoA2dF4wR0rMP5vR6dqBblaGAA5hN7GE2vPBHTDNZGJ6tZ9dnO6446dg9gGds\n"
      "eeadE1HdVnUj8nb+7CGm39vJ4fuNk9vogH0nMdxjCnXAinoOMRx8EklQsR747+Gz\n"
      "sxcdVYuNEb/E2vWBHTDNZGJ6tZC1JC9d6/RC3Vb1JC4tWnK9mk/Jw984ZuYugpMc\n"
      "1t9umoGFYCz0nMdxjCnXAbnoOMC5A0RxcWPzxfC5A0RH+j+mwoMxwhgfFY4EhVxp\n"
      "oCC8labNQhzRC3Vc1JC4tWnK9mpVA7k/o5LbuNpVA7S6AKAkjt17QzKJAiGWHakh\n"
      "RXwcoAndF4wPzxfC5A0RHponmwBHTDoM7GE2vPBHTDNZGJ6tZ9dnO6446dg9gGds\n"
      "eefdE1HcVnULbuNpVA7S6AKAkjxjCnt1gH0nMdxjCnXAinoOMRx8EklQsR747+Fz\n"
      "srm/VYaNEb/E2vPBHTDNZGJ6tZc1JC9d6/RC3Vc1JC4tWnK9mk/Jr984ZuYugpMc\n"
      "nt9uZTGFYCzZD0FFAA5NAC4i1PARStFycWPzxfC5A0RqodhswoMxwhgfFY4EhVx=\n"
      "-----END CERTIFICATE-----\n"

   Do not attempt to use this example directly, it has been filled with dummy data.

#. Create a :file:`certs` folder directly in the :file:`nrf_cloud_multi_service` folder, and copy :file:`client-cert.pem`, :file:`private-key.pem` and :file:`ca-cert.pem` into it.

   Be sure not to place the new folder in the :file:`nrf_cloud_multi_service/src` folder by accident.

   Now these certs are automatically used if the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option is enabled.

#. Build the sample with the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option enabled and the device ID you created configured.

   This is required for provisioning to succeed.

   Enable the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME` Kconfig option and set the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID` Kconfig option to the device ID.

   For example, if the device ID is ``698d4c11-0ccc-4f04-89cd-6882724e3f6f``:

   .. tabs::

      .. group-tab:: Bash

         .. code-block:: console

           west build --board your_board -p always -- -DCONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y -DCONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y -DCONFIG_NRF_CLOUD_CLIENT_ID="698d4c11-0ccc-4f04-89cd-6882724e3f6f"

      .. group-tab:: PowerShell

         .. code-block:: console

            west build --board your_board -p always -- -DCONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y -DCONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y  "-DCONFIG_NRF_CLOUD_CLIENT_ID=\`"698d4c11-0ccc-4f04-89cd-6882724e3f6f\`""

Configuration options
=====================

Set the following configuration options for the sample:

.. _CONFIG_MULTI_SERVICE_LOG_LEVEL_DBG:

CONFIG_MULTI_SERVICE_LOG_LEVEL_DBG - Sample debug logging
   Sets the log level for this sample to debug.

.. _CONFIG_POWER_SAVING_MODE_ENABLE:

CONFIG_POWER_SAVING_MODE_ENABLE - Enable Power Saving Mode (PSM)
   Requests Power Saving Mode from cellular network when enabled.

.. _CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS:

CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS - Cloud connection retry timeout (seconds)
   Sets the cloud connection retry timeout in seconds.

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
   Disable all location tracking methods to completely disable location tracking.
   Defaults to enabled.

.. _CONFIG_LOCATION_TRACKING_CELLULAR:

CONFIG_LOCATION_TRACKING_CELLULAR - Cellular location tracking
   Enables cellular location tracking.
   Disable all location tracking methods to completely disable location tracking.
   Defaults to enabled.

.. _CONFIG_LOCATION_TRACKING_WIFI:

CONFIG_LOCATION_TRACKING_WIFI - Wi-Fi location tracking
   Enables Wi-Fi location tracking.
   Disable all location tracking methods to completely disable location tracking.
   Requires the use of an nRF7002 companion chip.
   Defaults to disabled.

.. _CONFIG_TEMP_DATA_USE_SENSOR:

CONFIG_TEMP_DATA_USE_SENSOR - Use genuine temperature data
   Sets whether to take genuine temperature measurements from a connected BME680 sensor, or just simulate sensor data.

.. _CONFIG_TEMP_TRACKING:

CONFIG_TEMP_TRACKING - Track temperature data
   Enables tracking and reporting of temperature data to `nRF Cloud`_.
   Defaults to enabled.

.. _CONFIG_LED_INDICATION_PWM:

CONFIG_LED_INDICATION_PWM - PWM LED indication
   Use the `Zephyr pwm-leds`_ driver for LED indication.
   Defaults to enabled on the Thingy:91.

.. _CONFIG_LED_INDICATION_GPIO:

CONFIG_LED_INDICATION_GPIO - GPIO LED indication
   Use the `Zephyr gpio-leds`_ driver for LED indication.
   Defaults to enabled if there is a compatible devicetree entry, and the Thingy:91 is not the target.
   Defaults to enabled on the nRF91 Series DKs.

.. _CONFIG_LED_INDICATION_DISABLED:

CONFIG_LED_INDICATION_DISABLED - Completely disable LED indication
   Defaults to enabled if both :ref:`CONFIG_LED_INDICATION_PWM <CONFIG_LED_INDICATION_PWM>` and :ref:`CONFIG_LED_INDICATION_GPIO <CONFIG_LED_INDICATION_GPIO>` are disabled.

.. _CONFIG_LED_INDICATOR_RGB:

CONFIG_LED_INDICATOR_RGB - RGB LED status indication
   Use an on-board RGB LED for status indication.
   Defaults to enabled on the Thingy:91.

.. _CONFIG_LED_INDICATOR_4LED:

CONFIG_LED_INDICATOR_4LED - Four-LED status indication
   Use four discrete LEDs for status indication.
   Defaults to enabled if :ref:`CONFIG_LED_INDICATOR_RGB <CONFIG_LED_INDICATOR_RGB>` is disabled and LED indication is not disabled.

.. _CONFIG_LED_VERBOSE_INDICATION:

CONFIG_LED_VERBOSE_INDICATION - Verbose LED status indication
   Show more detailed LED status updates.
   Show a pattern when device messages are successfully sent, and when the initial connection to nRF Cloud is lost.
   Defaults to enabled if LED indication is enabled.

.. _CONFIG_LED_CONTINUOUS_INDICATION:

CONFIG_LED_CONTINUOUS_INDICATION - Continuous LED status indication
   Show an idle pattern, rather than turn the LEDs off, when the sample is idle.
   Defaults to enabled if LED indication is enabled.

.. _CONFIG_TEST_COUNTER:

CONFIG_TEST_COUNTER - Enable test counter
   Enables the test counter.

.. _CONFIG_AT_CMD_REQUESTS:

CONFIG_AT_CMD_REQUESTS - Enable AT command requests
   Allow remote execution of modem AT commands, requested using application-specific device messages.
   See :ref:`nrf_cloud_multi_service_remote_at` for details.

.. _CONFIG_AT_CMD_REQUEST_RESPONSE_BUFFER_LENGTH:

CONFIG_AT_CMD_REQUEST_RESPONSE_BUFFER_LENGTH - Length of AT command request response buffer (bytes)
   Sets the size of the buffer for storing responses to modem AT commands before they are forwarded to the cloud.
   Modem responses longer than this length will be replaced with an error code message (-NRF_E2BIG).
   Cannot be less than 40 bytes.

When using CoAP, the following additional configuration options are available:

.. _CONFIG_COAP_SHADOW_CHECK_RATE_SECONDS:

CONFIG_COAP_SHADOW_CHECK_RATE_SECONDS - Rate to check for shadow changes
   How many seconds between requests for any change (delta) in the device shadow.

.. _CONFIG_COAP_SHADOW_THREAD_STACK_SIZE:

CONFIG_COAP_SHADOW_THREAD_STACK_SIZE - CoAP Shadow Thread Stack Size (bytes)
   Sets the stack size (in bytes) for the shadow delta checking thread of the sample.

.. _CONFIG_NRF_CLOUD_COAP_FOTA:

CONFIG_NRF_CLOUD_COAP_FOTA - Enable FOTA with CoAP
   The sample periodically checks for pending FOTA jobs.
   The sample performs the FOTA update when received.

If :kconfig:option:`CONFIG_NRF_CLOUD_COAP_FOTA` is enabled, these options additional are available:

.. _CONFIG_COAP_FOTA_DL_TIMEOUT_MIN:

CONFIG_COAP_FOTA_DL_TIMEOUT_MIN - CoAP FOTA download timeout
    The time in minutes allotted for a FOTA download to complete.

.. _CONFIG_COAP_FOTA_USE_NRF_CLOUD_SETTINGS_AREA:

CONFIG_COAP_FOTA_USE_NRF_CLOUD_SETTINGS_AREA - Make FOTA compatible with other samples
   Use the same settings area as the nRF Cloud FOTA library.

.. _CONFIG_COAP_FOTA_SETTINGS_NAME:

CONFIG_COAP_FOTA_SETTINGS_NAME - Settings identifier
   Set the identifier for the CoAP FOTA storage if :kconfig:option:`CONFIG_COAP_FOTA_USE_NRF_CLOUD_SETTINGS_AREA` is not enabled.

.. _CONFIG_COAP_FOTA_SETTINGS_KEY_PENDING_JOB:

CONFIG_COAP_FOTA_SETTINGS_KEY_PENDING_JOB - Settings item key
   Set the settings item key for pending FOTA job info if :kconfig:option:`CONFIG_COAP_FOTA_USE_NRF_CLOUD_SETTINGS_AREA` is not enabled.

.. _CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES:

CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES - FOTA job check interval (minutes)
   How many minutes between requests for a FOTA job.

.. _CONFIG_COAP_FOTA_THREAD_STACK_SIZE:

CONFIG_COAP_FOTA_THREAD_STACK_SIZE - CoAP FOTA Thread Stack Size (bytes)
   Sets the stack size (in bytes) for the FOTA job checking thread of the sample.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

.. _nrf_cloud_multi_service_coap_provisioning:

Provisioning a CoAP device on nRF Cloud
***************************************

For more information, see `nRF Cloud Utilities documentation`_.
A device that connects to nRF Cloud using CoAP requires chained root CA certificates in the :kconfig:option:`CONFIG_NRF_CLOUD_SEC_TAG` security tag (``sec_tag``).
CoAP connectivity uses one root CA certificate, and HTTPS download for P-GPS and FOTA use the other.
The :file:`device_credentials_installer.py` script installs the chained root CA certificates when given the ``--coap`` option.
See below.

First, create a self-signed CA certificate using the following command:

.. code-block:: console

   python3 create_ca_cert.py -c US -st OR -l Portland -o "My Company" -ou "RD" -cn example.com -e admin@example.com -f CA

Remember to set all the variables with your own information.

You should now have three files:

* :file:`<certificate_serial>_ca.pem`
* :file:`<certificate_serial>_prv.pem`
* :file:`<certificate_serial>_pub.pem`

Where ``<certificate_serial>`` is your certificate's serial number in hex.
You will use these files to sign device certificates.

You only need to generate these three files once.
They can be used to sign as many device certificates as you need.

Next, for each device you wish to provision, execute the following command in the same folder as the above command:

.. note::
   The options used below assume that the sample is built with the default :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_IMEI` option enabled and :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX` option set to ``nrf-``.
   See :ref:`configuration_device_id` to use other device ID formats.

.. code-block:: console

   python3 device_credentials_installer.py --ca <certificate_serial>_ca.pem \
   --ca_key <certificate_serial>_prv.pem --id_str nrf- --id_imei -s -d --coap
   ...
   <- OK
   Saving provisioning endpoint CSV file provision.csv...
   Provisioning CSV file saved

Where ``<certificate_serial>`` is your certificate's serial number in hex.

Once the script creates the :file:`provision.csv` file, visit the `Provision Devices`_ page in the nRF Cloud portal to provision the device.
Provisioning registers the device with nRF Cloud, allowing it to use any nRF Cloud services through CoAP, MQTT, or REST connectivity.

.. _nrf_cloud_multi_service_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_multi_service`

.. include:: /includes/build_and_run_ns.txt

.. _nrf_cloud_multi_service_building_coap:

Building with CoAP Support
==========================

To build the sample to use CoAP instead of MQTT, use the ``-DOVERLAY_CONFIG=overlay_coap.conf`` option.

.. tabs::

   .. group-tab:: nRF9161 DK

      .. code-block:: console

          west build -p -b nrf9161dk_nrf9161_ns -- -DOVERLAY_CONFIG="overlay_coap.conf"

   .. group-tab:: nRF9160 DK

      .. code-block:: console

         west build -p -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG="overlay_coap.conf"

.. _nrf_cloud_multi_service_building_wifi_scan:

Building with nRF7002 EK Wi-Fi scanning support (for nRF91 Series DK)
=====================================================================

To build the sample with nRF7002 EK Wi-Fi scanning support, use the ``-DSHIELD=nrf7002ek``, ``-DEXTRA_DTC_OVERLAY_FILE=nrf91xxdk_with_nrf7002ek.overlay`` and  ``-DEXTRA_CONF_FILE=overlay-nrf7002ek-wifi-scan-only.conf`` options.

This enables the Wi-Fi location tracking method automatically.

.. tabs::

   .. group-tab:: nRF9161 DK

      .. code-block:: console

         west build -p -b nrf9161dk_nrf9161_ns -- -DSHIELD=nrf7002ek -DEXTRA_DTC_OVERLAY_FILE="nrf91xxdk_with_nrf7002ek.overlay" -DEXTRA_CONF_FILE="overlay-nrf7002ek-wifi-scan-only.conf"

   .. group-tab:: nRF9160 DK

      .. code-block:: console

         west build -p -b nrf9160dk_nrf9160_ns -- -DSHIELD=nrf7002ek -DEXTRA_DTC_OVERLAY_FILE="nrf91xxdk_with_nrf7002ek.overlay" -DEXTRA_CONF_FILE="overlay-nrf7002ek-wifi-scan-only.conf"

See also :ref:`the paragraphs on the Wi-Fi location tracking method <nrf_cloud_multi_service_wifi_location_tracking>`.

.. _nrf_cloud_multi_service_building_wifi_conn:

Building with experimental support for Wi-Fi connectivity for nRF5340 DK with nRF7002 EK (MQTT only)
====================================================================================================

This sample :ref:`experimentally <software_maturity>` supports connecting to nRF Cloud using Wi-Fi instead of using LTE.

An overlay for this is only provided for the nRF5340 DK with the nRF7002 EK shield attached.
It is possible to use Wi-Fi with other hardware combinations (such as the nRF7002 DK), but you must adjust heap and stack usage accordingly.
See the :file:`src/prj.conf` configuration file and the :file:`overlay_nrf7002ek_wifi_no_lte.conf` overlay for additional details.

Connecting to nRF Cloud using Wi-Fi is currently only possible with MQTT, and only by using the experimental and insecure Kconfig option :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES`.
See :ref:`nrf_cloud_multi_service_hardcoded_certs` for details on setting it up.

Once it is set up, you can configure your build to use Wi-Fi connectivity on the nRF5340 DK with the nRF7002 EK shield by using the ``--board nrf5340dk_nrf5340_cpuapp`` target and the ``-DSHIELD=nrf7002ek`` and ``-DOVERLAY_CONFIG=overlay_nrf7002ek_wifi_no_lte.conf`` options.
These options must be used in addition to the options required by the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option.

For example, for a device provisioned with the device ID ``698d4c11-0ccc-4f04-89cd-6882724e3f6f``:

.. tabs::

   .. group-tab:: Bash

      .. code-block:: console

        west build --board nrf5340dk_nrf5340_cpuapp -p always -- -DSHIELD=nrf7002ek -DOVERLAY_CONFIG=overlay_nrf7002ek_wifi_no_lte.conf -DCONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y -DCONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y -DCONFIG_NRF_CLOUD_CLIENT_ID="698d4c11-0ccc-4f04-89cd-6882724e3f6f"

   .. group-tab:: PowerShell

      .. code-block:: console

         west build --board nrf5340dk_nrf5340_cpuapp -p always -- -DSHIELD=nrf7002ek "-DOVERLAY_CONFIG=overlay_nrf7002ek_wifi_no_lte.conf" -DCONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y -DCONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y  "-DCONFIG_NRF_CLOUD_CLIENT_ID=\`"698d4c11-0ccc-4f04-89cd-6882724e3f6f\`""

Configuring Wi-Fi access point credentials
==========================================

This sample uses the :ref:`lib_wifi_credentials` library to manage Wi-Fi credentials.
Before the sample can connect to a Wi-Fi network, you must configure at least one credential set.

Once your device has been flashed with this sample, you can add a credential by connecting to your device's UART interface and then entering the following command:

.. code-block:: console

      wifi_cred add <NetworkSSID> WPA2-PSK <NetworkPassword>

Where ``<NetworkSSID>`` is replaced with the SSID of the Wi-Fi access point you want your device to connect to, and ``<NetworkPassword>`` is its password.
Then either reboot the device or use the ``wifi_cred auto_connect`` command to manually trigger a connection attempt.

From now on, these credentials will automatically be used when the configured network is reachable.

See the :ref:`Wi-Fi shell sample documentation <wifi_shell_sample>` for more details on the ``wifi_cred`` command.

Building with nRF Cloud logging support
=======================================

To enable transmission of `logs <Zephyr Logging_>`_ to nRF Cloud using the :ref:`lib_nrf_cloud_log` library, add the following parameter to your build command:

``-DOVERLAY_CONFIG=overlay_nrfcloud_logging.conf``

This overlay enables transmission of `logs <Zephyr Logging_>`_ to nRF Cloud.
Set the :kconfig:option:`CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL` Kconfig option to the log level of messages to send to nRF Cloud, such as ``4`` for debug log messages.

The overlay selects the :kconfig:option:`CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_TEXT` Kconfig option that enables log messages in JSON format.
You can read JSON log messages in real-time in the nRF Cloud user interface.
However, because JSON logs are large, you may want to edit the overlay file to change to using dictionary logging.
Deselect the :kconfig:option:`CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_TEXT` Kconfig option and select :kconfig:option:`CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_DICTIONARY` instead.
See `Dictionary-based Logging`_ to learn how dictionary-based logging works, how the dictionary is built, and how to decode the binary log output.

.. _nrf_cloud_multi_service_dependencies:

References
**********

* `RFC 7252 - The Constrained Application Protocol`_
* `RFC 7959 - Block-Wise Transfer in CoAP`_
* `RFC 7049 - Concise Binary Object Representation`_
* `RFC 8610 - Concise Data Definition Language (CDDL)`_
* `RFC 8132 - PATCH and FETCH Methods for CoAP`_
* `RFC 9146 - Connection Identifier for DTLS 1.2`_

Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_nrf_cloud`
* :ref:`lib_location`
* :ref:`lib_at_host`
* :ref:`lte_lc_readme`
* :ref:`lib_nrf_cloud_alert`
* :ref:`lib_nrf_cloud_log`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`CoAP <zephyr:networking_api>`
* :ref:`CoAP Client <zephyr:coap_client_interface>`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
