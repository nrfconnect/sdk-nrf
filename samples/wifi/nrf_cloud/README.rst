.. _wifi_nrf_cloud:

.. ncs-sample::
   :title: Wi-Fi: nRF Cloud

   The nRF Cloud sample demonstrates an nRF Cloud application running on nRF71 Series and nRF70 Series hardware, using Wi-Fi® as the transport.
   It integrates Wi-Fi-based location services, periodic sensor sampling, and more.
   It also demonstrates how to build connected, error-tolerant applications using Zephyr's ``conn_mgr`` connectivity management framework.

.. _wifi_nrf_cloud_requirements:

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires the following:

* A Wi-Fi access point with Internet access.
* An `nRF Cloud`_ account.
* The `nRF Cloud Utils`_ Python package, to provision and onboard the device (see :ref:`nrf_cloud_preconnect_prerequisites`).

.. include:: /includes/tfm.txt

.. _wifi_nrf_cloud_features:

Overview
********

This sample implements or demonstrates the following features:

* Error-tolerant use of the `nRF Cloud CoAP API`_ using the :ref:`lib_nrf_cloud_coap` library.
* Error-tolerant use of the `nRF Cloud MQTT API`_ using the :ref:`lib_nrf_cloud` library.
* Application Firmware Over-The-Air (FOTA) updates through `Memfault`_ Release Management, when built with the CoAP transport.
  See :ref:`wifi_nrf_cloud_fota` for details.
* Device monitoring and diagnostics through `Memfault`_, including periodic metrics and fault reporting.
  Available only when built with the CoAP transport.
  See :ref:`wifi_nrf_cloud_memfault` for details.
* Periodic Wi-Fi location tracking by scanning nearby access points and submitting them to nRF Cloud, using the :ref:`lib_location` library.
* Temperature measurements using the on-die temperature sensor, or simulated measurements on boards without one.
* Transmission of sensor samples to the nRF Cloud portal as `nRF Cloud Device Messages`_.
* Construction of valid `nRF Cloud Device Messages`_.
* Minimal LED status indication using the `Zephyr LED API`_.

.. _wifi_nrf_cloud_structure_and_theory_of_operation:

Structure and theory of operation
=================================

This sample is separated into a number of smaller functional units.
The top level functional unit and entry point for the sample is the :file:`src/main.c` file.
This file starts three primary threads, each with a distinct function:

* The cloud connection thread (``con_thread``, :file:`src/cloud_connection.c`) runs the :ref:`wifi_nrf_cloud_cloud_connection_loop`, which maintains a connection to `nRF Cloud`_.
* The application thread (``app_thread``, :file:`src/application.c`) runs the :ref:`wifi_nrf_cloud_application_thread_and_main_application_loop`, which controls demo features and submits `device messages <nRF Cloud Device Messages_>`_ to the :ref:`wifi_nrf_cloud_device_message_queue`.
* The message queue thread (``msg_thread``, :file:`src/message_queue.c`) then transmits these messages whenever there is an active connection.
  See :ref:`wifi_nrf_cloud_device_message_queue`.

:file:`src/main.c` also optionally starts a fourth thread, the ``led_thread``, which animates any onboard LEDs if :ref:`wifi_nrf_cloud_led_status_indication` is enabled.

When using CoAP, one additional thread starts:
The CoAP shadow delta checking thread (``coap_shadow``, :file:`src/shadow_support_coap.c`) runs the :ref:`wifi_nrf_cloud_coap_shadow_loop` which periodically asks nRF Cloud for any shadow changes.

.. _wifi_nrf_cloud_cloud_connection_loop:

Cloud connection loop
---------------------

The cloud connection loop (implemented in :file:`src/cloud_connection.c`) monitors network availability.
It starts a connection with `nRF Cloud`_ whenever the Internet becomes reachable, and closes that connection whenever Internet access is lost.
It has error handling and timeout features to ensure that failed or lost connections are re-established after a waiting period (:option:`CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS`).

Whenever a connection to nRF Cloud is started, the cloud connection loop follows the :ref:`nRF Cloud connection process <lib_nrf_cloud_connect>`.
The :ref:`lib_nrf_cloud` library handles most of the connection process, with exception to the following behavior:

When an MQTT device is first being associated with an nRF Cloud account, the `nRF Cloud MQTT API`_ will send a user association request notification to the device.
Upon receiving this notification, the device must wait for a user association success notification, and then manually disconnect from and reconnect to nRF Cloud.
Notifications of user association success are sent for every subsequent connection after this as well, so the device must only disconnect and reconnect if it previously received a user association request notification.
This behavior is handled by the ``NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST`` and ``NRF_CLOUD_EVT_USER_ASSOCIATED`` cases inside the :c:func:`cloud_event_handler` function.

When a CoAP device attempts to connect to nRF Cloud, the connection will fail to be authenticated until the device is onboarded to an nRF Cloud account.
See :ref:`wifi_nrf_cloud_provisioning` for details.

Upon startup, the cloud connection loop also updates the `device shadow <nRF Cloud Device Shadows_>`_.
This is performed in the :c:func:`update_shadow` function.

.. _wifi_nrf_cloud_device_message_queue:

Device message queue
--------------------

Any thread may submit `device messages <nRF Cloud Device Messages_>`_ to the device message queue (implemented in :file:`src/message_queue.c`), where they are stored until a working connection to `nRF Cloud`_ is established.
Once this happens, the message thread transmits all enqueued messages, one at a time, to nRF Cloud.
If an enqueued message fails to send, it is re-queued and retried later.
Transmission is paused whenever the connection to nRF Cloud is lost.
If more than :option:`CONFIG_MAX_CONSECUTIVE_SEND_FAILURES` messages in a row fail to send, the connection is reset and re-established after a delay (:option:`CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS`).

The following are sent directly rather than through the queue:

* `Device shadow <nRF Cloud Device Shadows_>`_ updates.
* Ground fix requests from the :ref:`lib_location` library.

.. _wifi_nrf_cloud_application_thread_and_main_application_loop:

Application thread and main application loop
--------------------------------------------

The application thread is implemented in the :file:`src/application.c` file, and is responsible for the high-level behavior of this sample.

When it starts, it logs the reset reason code.

It performs the following major tasks:

* Establishes periodic position tracking (which the :ref:`lib_location` library performs).
* Periodically samples temperature data (using the :file:`src/temperature.c` file).
* Constructs timestamped sensor sample and location `device messages <nRF Cloud Device Messages_>`_.
* Sends sensor sample and location device messages to the :ref:`wifi_nrf_cloud_device_message_queue`.

.. note::
   Periodic location tracking is handled by the :ref:`lib_location` library once it has been requested, whereas temperature samples are individually requested by the Main Application Loop.

.. _wifi_nrf_cloud_coap_shadow_loop:

CoAP shadow delta checking loop
-------------------------------

The CoAP shadow delta checking thread performs the following tasks:

* Checks for a change in the device shadow with the :c:func:`nrf_cloud_coap_shadow_get` function.
* Parse and process the JSON shadow delta document.
* If a change is received, the thread sends the change back with the :c:func:`nrf_cloud_coap_shadow_state_update` function.
  This is necessary to prevent the device from unnecessarily receiving the same shadow delta the next time through the loop.

.. _wifi_nrf_cloud_temperature_sensing:

Temperature sensing
-------------------

Temperature sensing is implemented in the :file:`src/temperature.c` file.
If the ``temp`` devicetree node is enabled, the on-die temperature sensor is read using the :ref:`zephyr:sensor` API.
This is the case for the nRF7120 and nRF54LM20 DKs.
On boards without an on-die temperature sensor available to the application core, such as the nRF7002 DK with nRF5430 as a host MCU, simulated temperature readings are generated instead.
For temperature readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_ (handled in :file:`src/cloud_connection.c`).

.. _wifi_nrf_cloud_location_tracking:

Location tracking
-----------------

Location tracking is handled in :file:`src/location_tracking.c`.
This sets up a periodic location request and passes results to a callback configured by :file:`src/application.c`.

For location readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_ (handled in :file:`src/cloud_connection.c`).

The device scans MAC addresses of nearby Wi-Fi access points and submits them to nRF Cloud to obtain a location estimate.

.. _wifi_nrf_cloud_led_status_indication:

LED status indication
---------------------

On boards that support LED status indication, this sample can indicate its current status with any on-board LEDs.

This is performed by a background thread implemented in the :file:`src/led_control.c` file.

Other threads may request either a temporary or indefinite LED pattern.
This wakes up the ``led_thread``, which begins animating the requested pattern, sleeping for 100 milliseconds at a time between animation frames, until the requested pattern has completed (if it is temporary), or until a new pattern is requested in its place.

This feature is enabled by default for the *board_target* mentioned in the `Requirements`_ sections.

The patterns displayed, the states they describe, and the options required for them to appear are as follows:

+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Status                                               | Pattern                                                                                                | Conditions                                                                                                |
+======================================================+========================================================================================================+===========================================================================================================+
| Trying to connect to nRF Cloud (for the first time)  | Single LED lit, spinning around the square of LEDs                                                     | LED status indication is enabled                                                                          |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Connection to nRF Cloud lost, reconnecting           | Single LED lit, spinning around the square of LEDs                                                     | The :option:`CONFIG_LED_VERBOSE_INDICATION` option is enabled                                             |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Fatal error                                          | All four LEDs blinking, 75% duty cycle                                                                 | LED status indication is enabled                                                                          |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Device message sent successfully                     | Alternating checkerboard pattern (two LEDs are lit at a time, either LED1 and LED4, or LED2 and LED3)  | The :option:`CONFIG_LED_VERBOSE_INDICATION` option is enabled                                             |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Idle                                                 | Single LED lit, bouncing between opposite corners (LED1 and LED4)                                      | The :option:`CONFIG_LED_CONTINUOUS_INDICATION` option is enabled                                          |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+

Under all other circumstances, on-board LEDs are turned off.

.. note::
   The nRF7002 DK has only two on-board LEDs, so LED status indication works only partially on that target.
   Patterns that require four LEDs (such as the "Device message sent successfully" and "Idle" patterns) will not display as described.

.. note::
  The :option:`CONFIG_LED_VERBOSE_INDICATION` and :option:`CONFIG_LED_CONTINUOUS_INDICATION` options are enabled by default.

See :ref:`wifi_nrf_cloud_customizing_LED_status_indication` for details on customizing the LED behavior.

See :ref:`wifi_nrf_cloud_led_third_party` for details on configuring LED status indication on third-party boards.

.. _wifi_nrf_cloud_memfault:

Memfault device monitoring
--------------------------

This sample uses `Memfault`_ (:kconfig:option:`CONFIG_MEMFAULT`) for general device monitoring and diagnostics whenever built with the CoAP transport.
See :ref:`ug_memfault` for an overview of the Memfault integration in the |NCS|.
By default, the sample reports the following data:

* Reboot events, including the reboot reason (:kconfig:option:`CONFIG_MEMFAULT_REBOOT_REASON_GET_HWINFO`).
* Coredumps on crash (:kconfig:option:`CONFIG_MEMFAULT_FAULT_HANDLER_RETURN`).
  Coredumps are stored in RAM and uploaded after the reboot (:kconfig:option:`CONFIG_MEMFAULT_RAM_BACKED_COREDUMP`).
  The ``nrf7002dk/nrf5340/cpuapp/ns`` board target reduces the coredump size to fit in RAM (see :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf`).
* Recent log lines, captured along with coredumps (:kconfig:option:`CONFIG_MEMFAULT_LOGGING_ENABLE`).
* Heartbeat metrics, collected every :kconfig:option:`CONFIG_MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS` seconds (one hour by default).
  The sample uses the built-in |NCS| metrics (:kconfig:option:`CONFIG_MEMFAULT_NCS_USE_DEFAULT_METRICS`) and does not define custom metrics.
  These include the following:

  * Wi-Fi connection metrics, such as connected time, disconnect count, RSSI, channel, and TX rate (:kconfig:option:`CONFIG_MEMFAULT_METRICS_WIFI`).
  * Memfault upload successes and failures (:kconfig:option:`CONFIG_MEMFAULT_METRICS_MEMFAULT_SYNC_SUCCESS`).
  * CPU usage and uptime (:kconfig:option:`CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE`).
  * CPU temperature, on boards with a temperature sensor available to the application core (:kconfig:option:`CONFIG_MEMFAULT_METRICS_CPU_TEMP`).
  * Heap usage (:kconfig:option:`CONFIG_MEMFAULT_METRICS_MEMORY_USAGE`).
  * Thread stack usage (:kconfig:option:`CONFIG_MEMFAULT_METRICS_THREADS`).
  * Boot time (:kconfig:option:`CONFIG_MEMFAULT_METRICS_BOOT_TIME`).

  See `Memfault: Built-in metrics for nRF Connect SDK`_ for the full list.

See :ref:`mod_memfault` for the configuration options of the Memfault integration.
To try out triggering crashes and defining custom metrics, see the :ref:`memfault_sample` sample.

The device appears on Memfault with its nRF Cloud device ID as serial.
This data is uploaded through nRF Cloud's CoAP endpoint (:kconfig:option:`CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP`), reusing the sample's existing nRF Cloud CoAP connection.
No Memfault project key is needed, as nRF Cloud forwards the data to Memfault.
See :ref:`wifi_nrf_cloud_testing_memfault` for how to view the data.

The Memfault SDK drives this upload on its own periodic-upload timer (:kconfig:option:`CONFIG_MEMFAULT_PERIODIC_UPLOAD`), independently of the sample's own logic.
To avoid this timer firing before the device actually has network or cloud connectivity, it is disabled by default (:kconfig:option:`CONFIG_MEMFAULT_PERIODIC_UPLOAD_ENABLED_DEFAULT`) and only enabled once the sample's cloud connection loop confirms connectivity (see the :c:func:`cloud_ready` function in the :file:`src/cloud_connection.c` file).
This same timer also drives FOTA checks when :ref:`wifi_nrf_cloud_fota` is enabled.

.. _wifi_nrf_cloud_memfault_buttons:

Memfault button actions
=======================

The periodic-upload timer fires only once in :kconfig:option:`CONFIG_MEMFAULT_PERIODIC_UPLOAD_INTERVAL_SECS`, and metrics heartbeats are only captured once in :kconfig:option:`CONFIG_MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS` (one hour by default).
To show how an application can drive Memfault on demand, the sample also uses the following DK buttons (:option:`CONFIG_SAMPLE_MEMFAULT_BUTTONS`, see the :file:`src/memfault_buttons.c` file) to trigger these actions:

* First button (``sw0`` devicetree alias) - Captures a metrics heartbeat (:c:func:`memfault_metrics_heartbeat_debug_trigger`) and posts all pending Memfault chunks (:c:func:`memfault_zephyr_port_post_data_return_size`).
* Second button (``sw1`` devicetree alias) - Checks for a FOTA update (:c:func:`memfault_zephyr_fota_start`), requires :ref:`wifi_nrf_cloud_fota` to be enabled.

Depending on the DK, these buttons are labeled as **Button 1** and **Button 2**, or **Button 0** and **Button 1**.
The sample logs the labels of the buttons at startup.
Button presses are ignored until the sample is connected to nRF Cloud.

.. note::
   Button actions are disabled on the ``nrf7002dk/nrf5340/cpuapp/ns``, ``nrf54lm20dk/nrf54lm20a/cpuapp/ns``, and ``nrf54lm20dk/nrf54lm20b/cpuapp/ns`` board targets.

.. _wifi_nrf_cloud_fota:

Memfault FOTA
-------------

This sample optionally supports application Firmware Over-The-Air (FOTA) updates, delivered through `Memfault`_ Release Management, when built with the CoAP transport (:kconfig:option:`CONFIG_NRF_CLOUD_COAP`).
Enable it with :option:`CONFIG_SAMPLE_MEMFAULT_FOTA`, which performs the following operations:

* Uses nRF Cloud CoAP as the download transport for Memfault (:kconfig:option:`CONFIG_MEMFAULT_USE_NRF_CLOUD_COAP`) and Memfault's FOTA Download backend (:kconfig:option:`CONFIG_MEMFAULT_ZEPHYR_FOTA_BACKEND_NCS`).
* Confirms the running image at startup (see :file:`src/memfault_fota_support.c`), so MCUboot does not roll back after a successful FOTA update reboot.
* Drives the actual FOTA check periodically and independently, through the Memfault SDK's own periodic-upload timer (:kconfig:option:`CONFIG_MEMFAULT_PERIODIC_FOTA_CHECK`), only once the app has confirmed nRF Cloud connectivity (see :kconfig:option:`CONFIG_MEMFAULT_PERIODIC_UPLOAD_ENABLED_DEFAULT`).
* Lets you trigger a FOTA check on demand with a button press (see :ref:`wifi_nrf_cloud_memfault_buttons`).
* Reboots automatically (through the sample's reboot helper) to apply a downloaded update.

This sample intentionally uses only Memfault's FOTA mechanism, and not nRF Cloud's native FOTA job-polling API, for application updates.

.. note::
   FOTA requires MCUboot with a devicetree layout that provides two full application image slots (``slot0_partition`` and ``slot1_partition``, each large enough to hold the secure and non-secure images together).
   This sample provides such dual-slot layout out of the box for the following board targets:

   * ``nrf7002dk/nrf5340/cpuapp/ns`` (``slot1_partition`` is placed in the board's external QSPI flash, since the 1 MB internal flash is too small to fit two full images).
   * ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` and ``nrf54lm20dk/nrf54lm20b/cpuapp/ns`` (both slots fit in internal flash).
   * ``nrf7120dk/nrf7120/cpuapp`` (secure-only build).

   The ``nrf7120dk/nrf7120/cpuapp/ns`` board target does not support FOTA.
   It builds with :kconfig:option:`SB_CONFIG_BOOTLOADER_NONE` instead of MCUboot (see :file:`boards/nrf7120dk_nrf7120_cpuapp_ns.conf`), so you cannot enable :option:`CONFIG_SAMPLE_MEMFAULT_FOTA` for that target.

   To build with FOTA enabled on a supported target, merge in both :file:`coap.conf` and :file:`coap-fota.conf` (see :ref:`wifi_nrf_cloud_building_coap`):

   ``-DEXTRA_CONF_FILE="coap.conf;coap-fota.conf"``

.. _wifi_nrf_cloud_test_counter:

Test counter
------------

Every time a sensor sample is sent, this counter is incremented and its current value is sent to `nRF Cloud`_.
A plot of the counter value over time appears automatically in the nRF Cloud portal, useful for visualizing connection loss, resets, and re-establishment.

You can enable or disable the counter using the device shadow's desired config section (``"counterEnable": true/false``), or force it always active with :option:`CONFIG_TEST_COUNTER`.

.. _wifi_nrf_cloud_device_message_formatting:

Device message formatting
-------------------------

This sample, when using MQTT to communicate with nRF Cloud, constructs JSON-based `device messages <nRF Cloud Device Messages_>`_.

While any valid JSON string can be sent as a device message, and accepted and stored by `nRF Cloud`_, there are some pre-designed message structures, known as schemas.
The nRF Cloud portal knows how to interpret these schemas.
These schemas are described in `nRF Cloud application protocols for long-range devices <nRF Cloud JSON protocol schemas_>`_.
The device messages constructed in the :file:`src/application.c` file all adhere to the general message schema.

Temperature data device messages conform to the ``temp`` ``deviceToCloud`` schemas.

This sample, when using CoAP to communicate with nRF Cloud, uses the :ref:`lib_nrf_cloud_coap` library to construct and transmit CBOR-based device messages.
Some CoAP traffic, specifically AWS shadow traffic, remains in JSON format.

.. _wifi_nrf_cloud_configuration:

Configuration
*************

|config|

Disabling key features
======================

You can independently disable the following key features of this sample by setting their respective Kconfig option disabled:

.. list-table::
   :widths: 50 50
   :header-rows: 1

   * - Feature
     - Kconfig option
   * - Location tracking
     - :option:`CONFIG_LOCATION_TRACKING`
   * - Temperature tracking
     - :option:`CONFIG_TEMP_TRACKING`
   * - Shadow handling when using CoAP
     - :option:`CONFIG_COAP_SHADOW`

For examples, see the :ref:`Configuration options <wifi_nrf_cloud_configuration>` section, or disable individual features using their respective Kconfig options.

.. _wifi_nrf_cloud_customizing_LED_status_indication:

Customizing LED status indication
=================================

To disable LED status indication (other than the selected idle behavior) after a connection to nRF Cloud has been established at least once, disable :option:`CONFIG_LED_VERBOSE_INDICATION`.

To turn the LED off while the sample is idle (rather than show an idle pattern), disable :option:`CONFIG_LED_CONTINUOUS_INDICATION`.

If you disable both of these options together, the status indicator LED remains off after a connection to nRF Cloud has been established at least once.

.. _wifi_nrf_cloud_led_third_party:

Configuring LED status indication for third-party boards
========================================================

This sample supports four discrete GPIO LEDs (:option:`CONFIG_LED_INDICATION_GPIO` and :option:`CONFIG_LED_INDICATOR_4LED`).
The board must have a compatible devicetree entry (``gpio-leds``).

To disable LED indication, enable :option:`CONFIG_LED_INDICATION_DISABLED`.

Useful debugging options
========================

To see all debug output for this sample, enable the ``CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL_DBG`` option.

See also the :ref:`wifi_nrf_cloud_test_counter`.

Configuration options
=====================

This sample uses the following Kconfig options:

.. options-from-kconfig::
   :show-type:

.. _wifi_nrf_cloud_setup_wifi_cred:

.. include:: /includes/wifi_credentials_shell.txt

.. important::
   Do not use any special characters in the SSID or password, such as spaces or quotes.

.. include:: /includes/wifi_credentials_static.txt

.. _wifi_nrf_cloud_configuration_files:

Configuration files
===================

The sample provides predefined configuration files for the following use cases:

* :file:`prj.conf` - General configuration file for all board targets.
* :file:`mqtt.conf` - Configuration file for the MQTT transport.
* :file:`coap.conf` - Configuration file for the CoAP transport, including `Memfault`_ device monitoring.
* :file:`coap-fota.conf` - Additional configuration file for :ref:`wifi_nrf_cloud_fota`.
  Must be used together with :file:`coap.conf`.
* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - Configuration file for the nRF7002 DK.
* :file:`boards/nrf54lm20dk_nrf54lm20a_cpuapp_ns.conf` and :file:`boards/nrf54lm20dk_nrf54lm20b_cpuapp_ns.conf` - Configuration files for the nRF54LM20 DK with the nRF7002-EB II shield.
* :file:`boards/nrf7120dk_nrf7120_cpuapp.conf` and :file:`boards/nrf7120dk_nrf7120_cpuapp_ns.conf` - Configuration files for the nRF7120 DK.
* :file:`sysbuild/mcuboot/` - Configuration of the MCUboot bootloader image used for FOTA.

Board and SoC files under the :file:`boards/` and :file:`socs/` folders are merged automatically for the selected target.
The :file:`mqtt.conf`, :file:`coap.conf`, and :file:`coap-fota.conf` files are not board-specific and must be passed with ``EXTRA_CONF_FILE``, as described in :ref:`wifi_nrf_cloud_building_coap`.

.. _wifi_nrf_cloud_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/nrf_cloud`

.. include:: /includes/build_and_run_ns.txt

.. _wifi_nrf_cloud_building_coap:

Selecting the transport: MQTT or CoAP
=====================================

The sample does not default to a transport.
You must select either MQTT or CoAP by adding one of the following parameters to your build command:

* For MQTT, add ``-DEXTRA_CONF_FILE=mqtt.conf``.
* For CoAP, add ``-DEXTRA_CONF_FILE=coap.conf``.

If you also want to enable :option:`CONFIG_SAMPLE_MEMFAULT_FOTA` on a board target that supports it (see :ref:`wifi_nrf_cloud_fota` for the list of supported targets), merge in the :file:`coap-fota.conf` file in addition to the :file:`coap.conf` file.
The :file:`coap-fota.conf` file only extends the CoAP transport and cannot be used on its own:

``-DEXTRA_CONF_FILE="coap.conf;coap-fota.conf"``

For example, to build the sample with the CoAP transport and FOTA support for the nRF7002 DK, use the following command:

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp/ns -- -DEXTRA_CONF_FILE="coap.conf;coap-fota.conf"

.. _wifi_nrf_cloud_provisioning:

Provisioning and onboarding
===========================

Before the sample can connect to nRF Cloud, you must provision the device and onboard it to your nRF Cloud account, as described in :ref:`lib_nrf_cloud_preconnect_provisioning`.
The sample supports all the provisioning methods described there.
For :ref:`hard-coded credentials <nrf_cloud_preconnect_hardcoded>`, copy the :file:`.pem` files into a :file:`certs` folder directly in the :file:`samples/wifi/nrf_cloud` folder (not in :file:`src`), which the sample adds to the include path.

.. _wifi_nrf_cloud_testing:

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset the kit.
#. Observe that the sample starts and logs the device ID, which you need for onboarding:

   .. code-block:: console

      <inf> main: Wi-Fi nRF Cloud sample has started, version: 1.0.0, protocol: CoAP
      ...
      <inf> nrf_cloud_info: Device ID: <device_id>

   If the device does not have nRF Cloud credentials yet, the sample logs the following message and waits:

   .. code-block:: console

      <wrn> cloud_connection: nRF Cloud credentials are not installed. Please install and reboot.

#. Provision and onboard the device as described in :ref:`wifi_nrf_cloud_provisioning`.
#. Add the credentials of your Wi-Fi access point, as described in :ref:`wifi_nrf_cloud_setup_wifi_cred`.
#. Reset the kit.
#. Observe that the sample connects to the Wi-Fi network, obtains the current date and time, and connects to nRF Cloud:

   .. code-block:: console

      <inf> cloud_connection: Network connectivity gained!
      <inf> cloud_connection: Network is ready
      <inf> cloud_connection: Waiting to obtain date/time
      <inf> cloud_connection: Connecting to nRF Cloud
      <inf> cloud_connection: Connected to nRF Cloud

   On boards that support :ref:`wifi_nrf_cloud_led_status_indication`, the LEDs show the connection status.
#. Except on the nRF7120 DK, where location tracking is disabled, observe that the sample periodically scans for nearby Wi-Fi access points and logs the location obtained from nRF Cloud:

   .. code-block:: console

      <inf> application: Location Updated: lat: <latitude>, lon: <longitude>, accuracy: <accuracy> m, Method: Wi-Fi

#. Observe that the sample periodically logs temperature readings:

   .. code-block:: console

      <inf> application: Die temperature is <temperature> C

#. Open the `nRF Cloud`_ portal, select your device on the `Devices <nRF Cloud Portal Devices_>`_ page, and observe that the temperature data and, except on the nRF7120 DK, the location data are displayed.
#. When using CoAP, observe that `Memfault`_ metrics, reboots, and faults for the device are reported to nRF Cloud.
   See :ref:`wifi_nrf_cloud_testing_memfault` for details.

.. _wifi_nrf_cloud_testing_memfault:

Testing Memfault device monitoring
----------------------------------

To test :ref:`wifi_nrf_cloud_memfault`, build and program the sample with the :file:`coap.conf` file, and onboard the device as described in the previous section.
Then, complete the following steps:

1. Upload the :file:`build/nrf_cloud/zephyr/zephyr.elf` file as a symbol file, using the `Memfault Dashboards`_ (**Software** > **Symbol Files**) or the `Memfault CLI tool`_.
   Memfault needs the symbol file to decode metrics and coredumps.
   It matches the symbol file to the firmware running on the device by its GNU build ID, so you do not need to link it to the device.
   Upload a new symbol file every time you rebuild the sample.
   See `Memfault: Upload a symbol file`_ for details.
#. Open the `Memfault Dashboards`_, go to **Fleet** > **Devices**, and select your device by its device ID.
#. Observe the data reported by the device in the following tabs:

   * **Processing Log** - Each piece of data received from the device, such as ``Received reboot event`` or ``Received Metric Report (Heartbeat)``, and processing errors.
     If the symbol file is missing, a ``Symbol file missing`` entry shows the GNU build ID of the expected symbol file.
   * **Reboots** - Reboot events with reboot reasons and software versions.
   * **Reports** - Heartbeat reports.
     Click :guilabel:`Expand All` to view the metric values of each report.
   * **Timeline** - Metrics and reboots over time.

   Coredumps from crashes are grouped as issues under **Monitoring** > **Issues**.
   Fleet-wide data is shown under **Fleet** > **Dashboards**.

By default, the device sends the first heartbeat report after one hour of uptime.
To see data sooner while testing, reduce the values of the :kconfig:option:`CONFIG_MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS` and :kconfig:option:`CONFIG_MEMFAULT_PERIODIC_UPLOAD_INTERVAL_SECS` Kconfig options.
Memfault might throttle devices that upload too often, which is shown as ``Device Reporting Throttled`` in the **Processing Log** tab.

.. _wifi_nrf_cloud_testing_fota:

Testing FOTA
------------

To test :ref:`wifi_nrf_cloud_fota`, build and program the sample with the :file:`coap.conf` and :file:`coap-fota.conf` files on a supported board target, and onboard the device as described in the previous section.
Then, complete the following steps:

1. Create a new version of the sample by increasing the ``PATCHLEVEL`` value in the :file:`VERSION` file.
#. Build the sample again, but do not program it.
#. Upload the :file:`build/nrf_cloud/zephyr/zephyr.signed.bin` file as a new release and deploy it to your device, using the `nRF Cloud`_ portal or the `Memfault CLI tool`_.
   See the `Memfault OTA Integration Guide`_ for details.
#. Wait for the device to check for updates.
   The first check runs within :kconfig:option:`CONFIG_MEMFAULT_PERIODIC_UPLOAD_INTERVAL_SECS` seconds plus one minute after the device connects to nRF Cloud.
   To shorten the wait while testing, reduce the value of this option.
#. Observe that the sample downloads the update and reboots to apply it:

   .. code-block:: console

      <inf> memfault_fota_support: FOTA update downloaded, rebooting to apply it

#. Observe that, after the reboot, MCUboot swaps the images and the sample confirms the new image and reports the new version:

   .. code-block:: console

      <inf> main: Memfault software_version: <new_version>
      <inf> memfault_fota_support: Confirming FOTA update image

.. _wifi_nrf_cloud_sample_output:

Sample output
=============

The following output is logged on the terminal when the sample is built with the CoAP transport and FOTA support, and a FOTA update is pending for the device:

.. code-block:: console

   [00:00:00.224,151] <inf> mflt: Periodic background upload scheduled - initial delay=7s period=10s
   [00:00:00.225,006] <inf> mflt: Reset Reason, HWINFO=0x0001
   [00:00:00.225,067] <inf> mflt: Reset Cause: Pin Reset
   [00:00:00.225,128] <inf> mflt: Reset Cause Stored: 0x000a
   [00:00:00.225,952] <inf> mflt: GNU Build ID: 9a7cca2db55f6d436522d814465b2149725a4a37
   [00:00:00.230,316] <inf> main: Wi-Fi nRF Cloud sample has started, version: 1.0.0, protocol: CoAP
   [00:00:00.230,346] <inf> main: Memfault software_version: 3.4.99+0
   [00:00:00.231,323] <inf> wifi_supplicant: wpa_supplicant initialized
   [00:00:00.232,025] <inf> cloud_connection: Setting up nRF Cloud library...
   [00:00:00.232,055] <inf> nrf_cloud_info: Device ID: DE2FB693AC38CD79
   [00:00:00.232,086] <inf> nrf_cloud_info: Protocol:          CoAP
   [00:00:00.232,116] <inf> nrf_cloud_info: Download protocol: CoAP
   [00:00:00.232,116] <inf> nrf_cloud_info: Sec tag:           16842753
   [00:00:00.232,116] <inf> nrf_cloud_info: CoAP JWT Sec tag:  16842753
   [00:00:00.232,147] <inf> nrf_cloud_info: Host name:         coap.nrfcloud.com
   [00:00:00.276,367] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
   [00:00:00.276,397] <inf> nrf_cloud_credentials: CA Size: 1792, AWS: Likely, CoAP: Likely
   [00:00:00.335,083] <inf> cloud_connection: Enabling connectivity...
   [00:00:00.335,113] <inf> cloud_connection: Waiting for network ready...
   [00:00:02.603,332] <inf> net_wifi_mgmt: Connection requested
   Connected
   [00:00:06.737,518] <inf> net_dhcpv4: Received: 192.168.0.125
   [00:00:06.738,098] <inf> cloud_connection: Network connectivity gained!
   [00:00:06.738,616] <inf> cloud_connection: Network is ready
   [00:00:06.738,647] <inf> cloud_connection: Waiting to obtain date/time
   [00:00:06.778,839] <inf> cloud_connection: Connecting to nRF Cloud
   [00:00:07.392,211] <inf> nrf_cloud_dns: Connected to server coap.nrfcloud.com via IPv4 address 23.22.110.24, port 5684
   [00:00:07.530,364] <inf> nrf_cloud_coap_transport: Request authorization with JWT
   [00:00:07.729,644] <inf> nrf_cloud_coap_transport: Authorization result_code: 2.01
   [00:00:07.730,163] <inf> nrf_cloud_coap_transport: Authorized
   [00:00:07.730,194] <inf> nrf_cloud_coap_transport: DTLS CID is active
   [00:00:08.139,343] <inf> cloud_connection: Connected to nRF Cloud
   [00:00:08.139,587] <inf> application: Waiting for device to determine current date and time
   [00:00:08.139,587] <inf> application: Current date and time determined
   [00:00:08.140,625] <inf> application: Temperature is 23 degrees C
   [00:00:08.140,869] <inf> shadow_support_coap: Checking for shadow delta...
   [00:00:08.255,645] <inf> shadow_config: Sending shadow reported configuration
   [00:00:13.465,179] <inf> location: LOCATION_REQ_MODE_ALL: all methods done
   [00:00:13.465,240] <inf> application: Location Updated: lat: <latitude>, lon: <longitude>, accuracy: 24.0 m, Method: Wi-Fi
   [00:00:17.766,479] <inf> mflt: FOTA Update Available. Starting Download with URL: https://ota-cdn.memfault.com/<path>
   [00:00:17.768,768] <inf> mflt: FOTA In Progress

.. _wifi_nrf_cloud_troubleshooting:

Troubleshooting
===============

If the sample does not connect to nRF Cloud, check the following:

* The device has valid nRF Cloud credentials installed and is onboarded to your nRF Cloud account with the device ID that the sample logs at startup.
  See :ref:`wifi_nrf_cloud_provisioning`.
* The credentials were installed for the transport the sample is built for.
  CoAP and MQTT use different root CA certificates.
* The Wi-Fi credentials configured for your device match your access point.
  See :ref:`wifi_nrf_cloud_setup_wifi_cred`.

If you have issues with Wi-Fi connectivity, see the :ref:`wifi_monitor_sample` sample documentation to learn how to capture and analyze Wi-Fi traffic to debug connectivity issues.

To get more detailed log output, see `Useful debugging options`_.

.. _wifi_nrf_cloud_dependencies:

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
* :ref:`lib_nrf_cloud_coap`
* :ref:`lib_nrf_cloud_credentials_keygen`
* :ref:`lib_location`
* :ref:`lib_date_time`
* :ref:`lib_hw_id`
* :ref:`mod_memfault`
* :ref:`lib_fota_download`
* :ref:`lib_dfu_target`

It uses the following Zephyr libraries:

* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
* :ref:`Wi-Fi credentials <zephyr:lib_wifi_credentials>`
* :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>`
* :ref:`CoAP <zephyr:networking_api>`
* :ref:`CoAP Client <zephyr:coap_client_interface>`

In addition, it uses the following secure firmware components:

* :ref:`Trusted Firmware-M <ug_tfm>`
* :ref:`MCUboot <mcuboot_index_ncs>`
