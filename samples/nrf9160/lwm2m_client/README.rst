.. _lwm2m_client:

nRF9160: LwM2M Client
#####################

.. contents::
   :local:
   :depth: 2

The LwM2M Client demonstrates usage of the :term:`Lightweight Machine to Machine (LwM2M)` protocol to connect a Thingy:91 or an nRF9160 DK to an LwM2M server through LTE.
This sample uses the :ref:`lib_lwm2m_client_utils` library.

Requirements
************

.. _supported boards:

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160_ns, nrf9160dk_nrf9160_ns

Additionally, the sample requires an activated SIM card, and an LwM2M server such as `Leshan Demo Server`_ or `Coiote Device Management`_ server.

Overview
********

LwM2M is an application layer protocol based on CoAP over UDP.
It is designed to expose various resources for reading, writing, and executing through an LwM2M server in a very lightweight environment.
The client sends data such as button and switch states, accelerometer data, temperature, and GPS position to the LwM2M server.
It can also receive activation commands such as buzzer activation and light control.

.. note::
   The GPS module interferes with the LTE connection.
   If GPS is not needed or if the device is in an area with poor connection, disable the GPS module.
   You can disable the GPS module by setting the configuration option :kconfig:`CONFIG_APP_GPS` to ``n``.


The following LwM2M objects are implemented in this sample:

.. list-table:: LwM2M objects
   :header-rows: 1

   *  - LwM2M objects
      - Object ID
      - Thingy:91
      - nRF9160 DK
   *  - LwM2M Server
      - 1
      - Yes
      - Yes
   *  - Device
      - 3
      - Yes
      - Yes
   *  - Connectivity Monitoring
      - 4
      - Yes
      - Yes
   *  - FOTA
      - 5
      - Yes
      - Yes
   *  - Location
      - 6
      - Yes
      - Yes
   *  - Accelerometer
      - 3313
      - Yes
      - Simulated
   *  - Color
      - 3335
      - Yes
      - Simulated
   *  - Temperature
      - 3303
      - Yes
      - Simulated
   *  - Pressure
      - 3323
      - Yes
      - Simulated
   *  - Humidity
      - 3304
      - Yes
      - Simulated
   *  - Generic Sensor
      - 3300
      - Yes
      - Simulated
   *  - Light Control
      - 3311
      - Yes
      - Yes
   *  - Push Button
      - 3347
      - Yes
      - Yes
   *  - Buzzer
      - 3338
      - Yes
      - No
   *  - On/Off Switch
      - 3342
      - No
      - Yes


.. note::

   The level resource on the Buzzer object does not work properly.
   This is due to an error in Zephyr’s float handling, which will be fixed soon.

.. _dtls_support:

DTLS Support
============

The sample has DTLS security enabled by default.
You need to provide the following information to the LwM2M server before you can make a successful connection:

* Client endpoint
* Identity
* `Pre-Shared Key (PSK)`_

See :ref:`server setup <server_setup_lwm2m>` for instructions on providing the information to the server.

Sensor simulation
=================

You can use the sample for obtaining actual sensor measurements or simulated sensor data for all sensors (including the accelerometer).
If the sample is running on the nRF9160 DK, only simulated sensor data is available, as it does not have any of the external sensors needed for actual measurements.

For example, you can enable the :kconfig:`CONFIG_ENV_SENSOR_USE_SIM` configuration option if you require simulated data from the temperature, humidity, pressure or gas resistance sensors.

.. _notifications_lwm2m:

Notifications
=============

LwM2M specifies the Notify operation, which can be used to notify the server of changes to the value of a resource field, for example, the measured value of a temperature sensor.
This allows active monitoring while using minimal bandwidth, as notifications can be sent with the updated values without the need for the server querying the client regularly.

To enable notifications, the server must initiate an observation request on one or more resources of interest.
For more information, see :ref:`notifications_setup_lwm2m`.

.. _sensor_module_lwm2m:

Sensor Module
=============

The sample has a sensor module which, if enabled, reads the selected sensors, and updates the client's resource values if it detects a sufficiently large change in one of the values.
The threshold for a sufficiently large change can be configured.
For example, a change in temperature of one degree Celsius.

Each sensor can be enabled separately.
The sampling period and change threshold of a sensor can also be configured independently of all the other sensors.

The sensor module is intended to be used together with notifications.
If notifications are enabled for a Sensor Value resource, and the corresponding sensor is enabled in the sensor module, a notification will be sent only when that value changes significantly (as specified by the change threshold).
Thus, the bandwidth usage can be significantly limited, while simultaneously registering important changes in sensor values.

See the :ref:`sensor_module_options` for information on enabling and configuring the sensor module.

.. note::

   When you track several resources and enable sensor module for several sensors , socket errors such as ``net_lwm2m_engine: Poll reported a socket error, 08`` and ``net_lwm2m_rd_client: RD Client socket error: 5`` might occur.

Configuration
*************

|config|

Setup
=====

Before building and running the sample, make sure that you complete the following steps:

1. Select the device to be tested.
2. Select the LwM2M server to be used for testing and register the device on it. You can also optionally enable notifications for the resources so that the resources are actively monitored by the server.
3. Configure the application to work with the chosen LwM2M server.

.. _server_setup_lwm2m:

Server setup
------------

The following instructions describe how to register your device to `Leshan Demo Server`_ or `Coiote Device Management server`_:

1. For adding the device to the LwM2M server, complete the following steps and for adding the device to an LwM2M bootstrap server, see the procedure in :ref:`registering the device to an LwM2M bootstrap server <bootstrap_server_reg>`:

   .. tabs::

      .. tab:: Leshan Demo Server

         1. Open the `Leshan Demo Server web UI`_.
         #. Click on :guilabel:`SECURITY` in the upper right corner in the UI.
         #. Click on :guilabel:`ADD SECURITY INFORMATION`.
         #. Enter the following data and click :guilabel:`ADD`:

            * Endpoint - urn\:imei\:*your Device IMEI*
            * Security Mode - psk
            * Identity: - urn\:imei\:*your Device IMEI*
            * Key - 000102030405060708090a0b0c0d0e0f

      .. tab:: Coiote Device Management

         1. Open `Coiote Device Management server`_.
         #. Click on :guilabel:`Device inventory` in the left menu in the UI.
         #. Click on :guilabel:`Add new device`.
         #. Click on :guilabel:`Connect your LwM2M device directly via the management server`.
         #. Enter the following data and click :guilabel:`Add device`:

            * Endpoint - urn\:imei\:*your Device IMEI*
            * Friendly Name - *recognizable name*
            * Security mode - psk (Pre-Shared Key)
            * Key - 000102030405060708090a0b0c0d0e0f

   .. _bootstrap_server_reg:

   For registering the device to an LwM2M bootstrap server, complete the following steps:

   .. tabs::

      .. tab:: Leshan Demo Server

         1. Open the `Leshan Boostrap Server Demo web UI <public Leshan Bootstrap Server Demo_>`_.
         #. Click on :guilabel:`BOOTSTRAP` in the top right corner.
         #. In the :guilabel:`BOOTSTRAP` tab, click on :guilabel:`ADD CLIENTS CONFIGURATION`.
         #. Click on :guilabel:`Add clients configuration`.
         #. Enter your Client Endpoint name - urn\:imei\:*your device IMEI*.
         #. Click :guilabel:`NEXT` and in the :guilabel:`LWM2M Server Configuration` section, enter the following data:

            * Server URL - ``coaps://leshan.eclipseprojects.io:5684``
            * Select :guilabel:`Pre-shared Key` as the :guilabel:`Security Mode`
            * Identity - urn\:imei\:*your device IMEI*
            * Key - ``000102030405060708090a0b0c0d0e0f``
         #. Click :guilabel:`NEXT` and in the :guilabel:`LWM2M Bootstrap Server Configuration` section enter the following data:

            * Server URL - ``coaps://leshan.eclipseprojects.io:5784``
            * Select :guilabel:`Pre-shared Key` as the :guilabel:`Security Mode`
            * Identity - urn\:imei\:*your device IMEI*
            * Key - ``000102030405060708090a0b0c0d0e0f``

            This information is used when your client connects to the server.
            If you choose :guilabel:`Pre-shared Key`, add the values for :guilabel:`Identity` and :guilabel:`Key` fields (the configured Identity or Key need not match the Bootstrap Server configuration).
            The same credentials will be provided in the :guilabel:`Leshan Demo Server Security configuration` page (see :ref:`dtls_support` for instructions).
            If :guilabel:`No Security` is chosen, no further configuration is needed.
            In this mode, no DTLS will be used for the communication with the LwM2M server.

         #. After adding values for the fields under both the :guilabel:`LwM2M Server Configuration` and :guilabel:`LwM2M Bootstrap Server Configuration` tabs, click :guilabel:`Add`.



      .. tab:: Coiote Device Management

         1. Open `Coiote Device Management server`_.
         #. Click on :guilabel:`Device inventory` in the menu on the left.
         #. Click on :guilabel:`Add new device`.
         #. Click on :guilabel:`Connect your LwM2M device via the Bootstrap server`.
         #. Enter the following data and click :guilabel:`Configuration`:

            * Endpoint - urn\:imei\:*your Device IMEI*
            * Friendly Name - *recognisable name*
            * Security mode - psk (Pre-Shared Key)
            * Key - 000102030405060708090a0b0c0d0e0f

         #. Click :guilabel:`Add device`.

.. note::

   The :guilabel:`Client Configuration` page of the LWM2M Bootstrap server and the :guilabel:`Registered Clients` page of the LWM2M server display only a limited number of devices by default.
   You can increase the number of displayed devices from the drop-down menu associated with :guilabel:`Rows per page`.
   In both cases, the menu is displayed at the bottom-right corner of the :guilabel:`Client Configuration` pages.

2. Set the server address in the client:

   a. Open :file:`src/prj.conf`.
   #. Set :kconfig:`CONFIG_APP_LWM2M_SERVER` to the correct server URL:

      * For `Leshan Demo Server`_ - ``leshan.eclipseprojects.io`` (`public Leshan Demo Server`_).
      * For `Coiote Device Management`_ - ``eu.iot.avsystem.cloud`` (`Coiote Device Management server`_).

   #. Set :kconfig:`CONFIG_LWM2M_PEER_PORT` to the port number used by the server you have chosen.
      Remember to set it to the port number used by the bootstrap server if bootstrap is enabled.

#. Set :kconfig:`CONFIG_APP_LWM2M_PSK` to the hexadecimal representation of the PSK used when registering the device with the server.



.. _notifications_setup_lwm2m:

Enabling notifications
----------------------

Enabling notifications for a resource varies slightly from server to server.
The client must be connected to the server to enable notifications.
Following are the instructions for enabling notifications in the Leshan Demo server and Coiote Device Management server:

.. tabs::

   .. tab:: Leshan Demo Server

      1. Open the `Leshan Demo Server web UI`_.
      #. Identify your device in the :guilabel:`Clients` tab and select it.
      #. Select the desired object in the menu on the left in the UI.
      #. Identify one or more resources you want to track and click :guilabel:`OBS` next to it.

         * You can track either a single resource or all the resources of an object. It is recommended to track only the resources that are expected to change.
         * If you want to use the :ref:`sensor_module_lwm2m`, at least the Sensor Value resource must be tracked for all sensors enabled in the Sensor Module.

   .. tab:: Coiote Device Management server

      1. Open `Coiote Device Management server`_.
      #. Click :guilabel:`Device inventory` tab in the top.
      #. Identify your device in the list and click on the anchor text corresponding to the device ID in the :guilabel:`Identity column`.
      #. Click on the Objects tab in the new menu to the left, just below :guilabel:`Dashboard`.
      #. Identify one or more objects that you want to receive notifications from, and expand it by clicking on them.
      #. Identify one or more resources of the object that you want to track:

         * You can track either a single resource or all the resources of an object. It is recommended to track only the resources that are expected to change.
         * If you want to use the :ref:`sensor_module_lwm2m`, at least the Sensor Value resource must be tracked for all sensors enabled in the Sensor Module.

      #. Click on the :guilabel:`Value Tracking` button of the selected resource.
      #. Select :guilabel:`Observe` or :guilabel:`Monitoring` from the dropdown menu.

         * Selecting :guilabel:`Observe` will only update the Value field of the resource when it receives a notification.
         * Selecting :guilabel:`Monitoring` will additionally create a graph of the logged datapoints.

      #. Click on :guilabel:`Limit data usage` to configure how often notifications are sent.

Configuration options
=====================

Check and configure the following configuration options for the sample:

Server options
--------------

.. option:: CONFIG_APP_LWM2M_SERVER - Configuration for LwM2M server URL

   The sample configuration sets the URL of the LwM2M server to be used. The URL must not be prefixed with the application protocol.

.. option:: CONFIG_APP_LWM2M_PSK - Configuration for Pre-Shared Key

   The sample configuration is used to set the hexadecimal representation of the PSK used when registering the device with the server.

.. option:: CONFIG_APP_ENDPOINT_PREFIX - Configuration for setting prefix for endpoint name

   This configuration option changes the prefix of the endpoint name.

LwM2M objects options
---------------------

.. option:: CONFIG_APP_TEMP_SENSOR - Configuration for enabling an LwM2M Temperature sensor object

   The sample configuration is used to enable an LwM2M Temperature sensor object.
   All compatible objects are enabled by default.
   Disabled objects will not be visible in the server.
   This configuration option can be used for other LwM2M objects also by modifying the option accordingly.

.. option:: CONFIG_APP_GPS - Configuration for enabling GPS functionality

   The sample configuration is used to enable the GPS.
   This configuration might interfere with LTE if the GPS conditions are not optimal.
   Disable this option if GPS is not needed.

.. option::  CONFIG_GPS_PRIORITY_ON_FIRST_FIX - Configuration for prioritizing GPS

   The configuration is used to prioritize GPS over LTE during the search for first fix.
   Enabling this option makes it significantly easier for the GPS module to find a position but will also affect performance for the rest of the application during the search for first fix.

.. option:: CONFIG_ENV_SENSOR_USE_SIM - Configuration to enable simulated sensor data

   The configuration when enabled, makes the sensor returns simulated data and not actual measurements.
   This option is available for all sensors, including the accelerometer.

.. option:: CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_0 - Configuration for selecting the IPSO Color sensor object version

   The configuration option sets the version of the OMA IPSO object specification that is to be used by the user defined Color sensor IPSO object to 1.0.

.. option:: CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1 - Configuration for selecting the IPSO Color sensor object version

   The configuration option sets the version of the OMA IPSO object specification that is to be used by the user defined Color sensor IPSO object to 1.1.

.. option:: CONFIG_APP_CUSTOM_VERSION - Configuration to set custom application version reported in the Device object.

   The configuration option allows to specify custom application version reported to the LwM2M server. By default, the current |NCS| version is used.

.. _sensor_module_options:

Sensor module options
---------------------

.. option:: CONFIG_SENSOR_MODULE - Configuration for periodic sensor reading

   This configuration option enables periodic reading of sensors and updating the resource values when
   the change is sufficiently large.
   The server is notified if a change in one or more resources is observed.

.. option:: CONFIG_SENSOR_MODULE_TEMP - Configuration to enable Temperature sensor

   This configuration option enables the Temperature sensor in the Sensor Module.

.. option:: CONFIG_SENSOR_MODULE_TEMP_PERIOD - Configuration for interval between sensor readings

   This configuration option sets the time interval (in seconds) between sensor readings from the Temperature sensor.

.. option:: CONFIG_SENSOR_MODULE_TEMP_DELTA_INT - Configuration for setting required change

   This configuration option sets the required change (integer part) in sensor value before the corresponding resource value is updated.

.. option:: CONFIG_SENSOR_MODULE_TEMP_DELTA_DEC - Configuration for setting required change

   This configuration option sets the required change (decimal part) in sensor value before the corresponding resource value is updated.

.. note::

   You can use the configuration options for different sensor types by modifying the configuration options accordingly.

Additional configuration
========================

Check and configure the following LwM2M options that are used by the sample:

* :kconfig:`CONFIG_LWM2M_PEER_PORT` - LwM2M server port
* :kconfig:`CONFIG_LWM2M_ENGINE_MAX_OBSERVER` - Maximum number of resources that can be tracked. This must be increased if you want to observe more than 10 resources.
* :kconfig:`CONFIG_LWM2M_ENGINE_MAX_MESSAGES` - Maximum number of LwM2M message objects. This value must be increased if many notifications will be sent at once.
* :kconfig:`CONFIG_LWM2M_ENGINE_MAX_PENDING` - Maximum number of pending LwM2M message objects. The value needs to be increased if many notifications will be sent at once.
* :kconfig:`CONFIG_LWM2M_ENGINE_MAX_REPLIES` - Maximum number of LwM2M reply objects. The value needs to be increased if many notifications will be sent at once.
* :kconfig:`CONFIG_LWM2M_COAP_BLOCK_SIZE` - Increase if you need to add several new LwM2M objects to the sample, as the registration procedure contains information about all the LwM2M objects in one block.
* :kconfig:`CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME` - Set this option to configure how often the client sends ``I'm alive`` messages to the server.
* :kconfig:`CONFIG_LWM2M_IPSO_TEMP_SENSOR_VERSION_1_0` - Sets the IPSO Temperature sensor object version to 1.0. You can use this configuration option for other IPSO objects also by modifying the option accordingly. See the `LwM2M Object and Resource Registry`_ for a list of objects and their available versions.
* :kconfig:`CONFIG_LWM2M_IPSO_TEMP_SENSOR_VERSION_1_1` - Sets the IPSO Temperature sensor object version to 1.1. You can use this configuration option for other IPSO objects also by modifying the option accordingly. See the `LwM2M Object and Resource Registry`_ for a list of objects and their available versions.

.. note::
   Changing lifetime might not work correctly if you set it to a value beyond 60 seconds.
   It might cause resending message error and eventually timeout.

For Thingy:91, configure the ADXL362 accelerometer sensor range by choosing one of the following options (default value is |plusminus| 2 g):

* :kconfig:`CONFIG_ADXL362_ACCEL_RANGE_2G` - Sensor range of |plusminus| 2 g.

* :kconfig:`CONFIG_ADXL362_ACCEL_RANGE_4G` - Sensor range of |plusminus| 4 g.

* :kconfig:`CONFIG_ADXL362_ACCEL_RANGE_8G` - Sensor range of |plusminus| 8 g.

Resolution depends on range: |plusminus| 2 g has higher resolution than |plusminus| 4 g, which again has higher resolution than |plusminus| 8 g.

Configuration files
===================

The sample provides predefined configuration files for typical use cases.

The following files are available:

* ``prj.conf`` - Standard default configuration file
* ``overlay-queue`` - Enables LwM2M Queue Mode support
* ``overlay-bootstrap.conf`` - Enables LwM2M bootstrap support

The sample can either be configured by editing the :file:`prj.conf` file and the relevant overlay files, or through menuconfig or guiconfig.

.. _build_lwm2m:

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/lwm2m_client`

.. include:: /includes/build_and_run_nrf9160.txt


After building and running the sample, you can locate your device in the server:

   * Leshan - Devices are listed under :guilabel:`Clients`.
   * Coiote - Devices are listed under :guilabel:`Device inventory`.


Queue Mode support
==================

To use the LwM2M Client with LwM2M Queue Mode support, build it with the ``-DOVERLAY_CONFIG=overlay-queue.conf`` option:

.. code-block:: console

   west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-queue.conf

Bootstrap support
=================

To successfully run the bootstrap procedure, the device must first be registered in the LwM2M bootstrap server.
See :ref:`Registering your device to an LwM2M boot strap server <bootstrap_server_reg>` for instructions.

To build the LwM2M Client with LwM2M bootstrap support, build it with the ``-DOVERLAY_CONFIG=overlay-bootstrap.conf`` option:

.. code-block:: console

   west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-bootstrap.conf

See :ref:`cmake_options` for instructions on how to add this option.
Keep in mind that the used bootstrap port is set in the aforementioned configuration file.

Testing
=======

|test_sample|

   #. |connect_kit|
   #. |connect_terminal|
   #. Observe that the sample starts in the terminal window.
   #. Check that the device is connected to the chosen LwM2M server.
   #. Press **Button 1** on nRF9160 DK or **SW3** on Thingy:91 and confirm that the button event appears in the terminal.
   #. Check that the button press event has been registered on the LwM2M server by confirming that the press count has been updated.
   #. Retrieve sensor data from various sensors and check if values are reasonable.
   #. Test GPS module:

      a. Ensure that :kconfig:`CONFIG_GPS_PRIORITY_ON_FIRST_FIX` is enabled.
      #. Ensure that you are in a location with good GPS signal, preferably outside.
      #. Wait for the GPS to receive a fix, which will be displayed in the terminal.
         It might take several minutes for the first fix.

   #. Try to enable or disable some sensors in menuconfig and check if the sensors
      appear or disappear correspondingly in the LwM2M server.

Firmware Over-the-Air (FOTA)
============================

You can update the firmware of the device if you are using Coiote Device Management server or Leshan server.
Application firmware updates and modem firmware (both full and delta) updates are supported.

To update the firmware, complete the following steps:

   1. Identify the firmware image file to be uploaded to the device. See :ref:`lte_modem` and :ref:`nrf9160_fota` for more information.
   #. Open `Coiote Device Management server`_ and click :guilabel:`LwM2M firmware`.
   #. Click :guilabel:`Schedule new firmware upgrade`.
   #. Click :guilabel:`Upload file` in the bottom left corner and upload the firmware image file.
   #. Configure the necessary firmware update settings in the menu to the right.
   #. Click :guilabel:`Upgrade`.
   #. Observe in the terminal window that the image file is being downloaded.
      The download will take some time.
      If the server lifetime is not increased, Coiote might drop the connection to the device. The device reconnects later.
   #. When the download is complete, the device restarts on its own after installing the firmware.
      Restart the device manually if it has not started automatically.
      The device runs the updated firmware and reconnects to Coiote Device Management server automatically.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

* :ref:`modem_info_readme`
* :ref:`at_cmd_parser_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`lte_lc_readme`
* :ref:`lib_date_time`
* :ref:`lib_dfu_target`
* :ref:`lib_fmfu_fdev`
* :ref:`lib_fota_download`
* :ref:`lib_download_client`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`gpio_api`
* :ref:`pwm_api`
* :ref:`sensor_api`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
