.. _asset_tracker_v2:

nRF9160: Asset Tracker v2
#########################

.. contents::
   :local:
   :depth: 2

The Asset Tracker v2 is a real-time configurable ultra-low power capable application firmware for the nRF9160 System in Package (SiP).
It is a complete rework of the :ref:`asset_tracker` application.
This application introduces a set of new features, which are not present in the :ref:`asset_tracker` application:

* Ultra-low power by design - The application highlights the power saving features of the nRF9160 SiP, which is critical for successfully developing small form-factor devices and products which need very long battery lifetime.
* Offline first - Highly-mobile cellular IoT products need to handle unreliable connections gracefully by implementing mechanisms to retry the failed sending of data.
* Timestamping on the device - Sensor data is timestamped on the device using multiple time sources. When the device is offline (planned or unplanned), the timestamping does not rely on the cloud side.
* Batching of data - Data can be batched to reduce the number of messages transmitted, and to be able to retain collected data while the device is offline.
* Configurable at run time - The application behavior (for example, accelerometer sensitivity or GPS timeout) can be configured at run time. This improves the development experience with individual devices or when debugging the device behavior in specific areas and situations. It also reduces the cost for transmitting data to the devices by reducing the frequency of sending firmware updates to the devices.

Implementation of the above features required a rework of the existing application.
Hence, this application is not backward compatible to the :ref:`asset_tracker` application.

.. note::
    The code is currently a work in progress and is not fully optimized yet. It will undergo changes and improvements in the future.

Overview
********

The application samples sensor data and publishes the data to a connected cloud service over TCP/IP via LTE.
As of now, the application supports the following cloud services and the corresponding cloud-side instances:

+---------------------------+---------------------------------------------------------------------------------------+
| Cloud service             | Cloud-side instance                                                                   |
+===========================+=======================================================================================+
| `AWS IoT Core`_           | `nRF Asset Tracker for AWS`_                                                          |
+---------------------------+---------------------------------------------------------------------------------------+
| `Azure IoT Hub`_          | `nRF Asset Tracker for Azure`_                                                        |
+---------------------------+---------------------------------------------------------------------------------------+
| `nRF Cloud`_              | `nRF Cloud Device API`_ (See `nRF Cloud documentation`_.)                             |
+---------------------------+---------------------------------------------------------------------------------------+

Firmware architecture
=====================

The Asset Tracker v2 application has a modular structure, where each module has a defined scope of responsibility.
The application makes use of the :ref:`event_manager` to distribute events between modules in the system.
The event manager is used for all the communication between the modules.
A module converts incoming events to messages and processes them in a FIFO manner.
The processing happens either in a dedicated processing thread in the module, or directly in the event manager callback.

The following figure shows the relationship between the modules and the event manager.
It also shows the modules with thread and the modules without thread.

.. figure:: /images/asset_tracker_v2_module_hierarchy.svg
    :alt: Module hierarchy

    Relationship between modules and the event manager

See :ref:`asset_tracker_v2_internal_modules` for more information.

Data types
==========

Data from multiple sensor sources are collected to construct information about the location, environment, and the health of the nRF9160-based device.
The application supports the following data types:

.. _app_data_types:

+----------------+----------------------------+-----------------------------------------------+
| Data type      | Description                | Identifiers                                   |
+================+============================+===============================================+
| Location       | GNSS coordinates           | APP_DATA_GNSS                                 |
+----------------+----------------------------+-----------------------------------------------+
| Environmental  | Temperature, humidity      | APP_DATA_ENVIRONMENTAL                        |
+----------------+----------------------------+-----------------------------------------------+
| Movement       | Acceleration               | APP_DATA_MOVEMENT                             |
+----------------+----------------------------+-----------------------------------------------+
| Modem          | LTE link data, device data | APP_DATA_MODEM_DYNAMIC, APP_DATA_MODEM_STATIC |
+----------------+----------------------------+-----------------------------------------------+
| Battery        | Voltage                    | APP_DATA_BATTERY                              |
+----------------+----------------------------+-----------------------------------------------+
| Neighbor cells | Neighbor cell measurements | APP_DATA_NEIGHBOR_CELLS                       |
+----------------+----------------------------+-----------------------------------------------+

The sets of sensor data that are published to the cloud service consist of relative `timestamps <Timestamping_>`_ that originate from the time of sampling.

Device modes
============

The application can be either in an active or in a passive state depending on the applied device mode.
The device mode is a part of the application's real-time configurations.
The device modes and their descriptions are listed in the following table:

+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Real-time Configurations       | Description                                                                                                                          | Default values |
+================================+======================================================================================================================================+================+
| Device mode                    | Either in active or passive mode.                                                                                                    | Active         |
+----------+---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|  Active  |                     | Sample and publish data at regular intervals.                                                                                        |                |
|          +---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Active wait time    | Number of seconds between each sampling/publication.                                                                                 | 120 seconds    |
+----------+---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|  Passive |                     | Sample and publish data only if movement has been detected.                                                                          |                |
|          +---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Movement resolution | Sample and publish data after detecting movement.                                                                                    |                |
|          |                     | Wait for a duration specified by the parameter until a movement triggers the next update.                                            | 120 seconds    |
|          +---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Movement timeout    | Sample and publish data at a minimum of the time interval specified by the parameter. Not dependent on movement.                     | 3600 seconds   |
+----------+---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| GPS timeout                    | Timeout for acquiring a GPS fix during sampling of the data.                                                                         | 60 seconds     |
+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Accelerometer threshold        | Accelerometer threshold in m/s². Minimal absolute value in m/s² for the accelerometer readings to be considered as a valid movement. | 10 m/s²        |
+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| No Data List (NOD)             | List containing :ref:`data types <app_data_types>` that is not included when the application samples data.                           | No entries     |
|                                | Currently, the application supports only APP_DATA_GNSS and APP_DATA_NEIGHBOR_CELLS.                                                  | (Request all)  |
+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+

See :ref:`Kconfig options for default device configuration values <default_config_values>` for a list of configuration options that can set the default values of the device configuration parameters.

.. note::
   The configurations that are used depend on the application state.
   For instance, in active mode, the ``Movement resolution`` and the ``Movement timeout`` parameters are not used.

The following flow charts shows the functioning of the application in active and passive states.
The charts also show the relationship between data sampling, publishing, and device configurations.
All the configurations that are not essential to this relationship are abstracted for simplicity.

.. figure:: /images/asset_tracker_v2_active_state.svg
    :alt: Active state flow chart

    Active state flow chart

In the active state, the application samples and publishes data at regular intervals that are set by the ``Active wait timeout`` configuration.

.. figure:: /images/asset_tracker_v2_passive_state.svg
    :alt: Passive state flow chart

    Passive state flow chart

In the passive state, the application will only sample and publish upon movement.
This reduces the amount of data transferred over the air and the numerous processing cycles.
The flow chart does not include the timer that acts on the ``Movement timeout`` configuration.
This timer is enabled when the application enters the passive state.
When the timer expires, the application will initiate data sampling and publishing.
The timer ensures that if there is no movement, the application still sends updates to the cloud service.
The timeout acts as a failsafe in the case of zero movement for the asset wearing the tracker over a long time.
Ideally, the ``Movement timeout`` parameter should be set to a value much higher than the value of ``Movement resolution``.

The device retrieves its real-time configurations from the cloud service in either of the following ways:

* Upon every established connection to the cloud service, the application will always request its cloud-side device state that contains the latest real-time configurations.
* When the device exits `Power Saving Mode (PSM)`_ to publish data, and if the cloud-side device configuration has been updated while the device was in PSM, the application will request for the newly changed configuration.

The application always acknowledges any newly applied device configurations back to the cloud service.

.. note::
   The application always stores any new configuration obtained from the cloud service to the flash memory.
   If the device reboots unexpectedly in areas without LTE coverage, the application will have access to the configuration that was applied last.

Data buffers
============

Data sampled from the onboard modem and the external sensors is stored in ring buffers.
Newly sampled data is always published prior to the old, buffered data.

The application has LTE and cloud connection awareness.
Upon a disconnect from the cloud service, the application keeps the sensor data that has been buffered and empty the buffers in batch messages when the application reconnects to the cloud service.

User interface
**************

The application uses the following buttons on the nRF9160-based development kits:

* Button 1 on Thingy:91
* Button 1 and Button 2 on nRF9160 DK

Additionally, the application displays LED behavior that corresponds to the task performed by the application.
The following table shows the purpose of each supported button:

+--------+-----------------------------------+---------------------------------------------------------------------------------------------+
| Button | Thingy:91                         | nRF9160 DK                                                                                  |
+========+===================================+=============================================================================================+
| 1      | Send message to the cloud service | Send message to the cloud service.                                                          |
+--------+-----------------------------------+---------------------------------------------------------------------------------------------+
| 2      |                                   | Send message to the cloud service.                                                          |
|        |                                   +---------------------------------------------------------------------------------------------+
|        |                                   | Fake movement. No external accelerometer in nRF9160 DK to trigger movement in passive mode. |
+--------+-----------------------------------+---------------------------------------------------------------------------------------------+

The following table shows the LED behavior demonstrated by the application:

+---------------------------+-------------------------+-----------------------+
| State                     | Thingy:91 RGB LED       | nRF9160 DK solid LEDs |
+===========================+=========================+=======================+
| LTE connection search     | Yellow, blinking        | LED1 blinking         |
+---------------------------+-------------------------+-----------------------+
| GPS fix search            | Purple, blinking        | LED2 blinking         |
+---------------------------+-------------------------+-----------------------+
| Publishing data           | Green, blinking         | LED3 blinking         |
+---------------------------+-------------------------+-----------------------+
| Active mode               | Light blue, blinking    | LED4 blinking         |
+---------------------------+-------------------------+-----------------------+
| Passive mode              | Dark blue, slow blinking| LED4 slow blinking    |
+---------------------------+-------------------------+-----------------------+
| Error                     | Red, on                 | all 4 LEDs blinking   |
+---------------------------+-------------------------+-----------------------+
| Completion of FOTA Update | White, rapid blinking   | all 4 LEDs on         |
+---------------------------+-------------------------+-----------------------+

Requirements
************

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns, nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

Configuration
*************

The application has a Kconfig file with options that are specific to the Asset Tracker v2, where each of the modules has a separate submenu.
These options can be used to enable and disable modules and modify their behavior and properties.
|config|

Setup
=====

The application is designed to support communication with different cloud services, a single service at a time.
Currently, the application supports the following services and technologies in the connection:

+--------------------------+---------------------------------+
| Service                  | Technologies                    |
+==========================+=================================+
| `AWS IoT Core`_          |   `MQTT`_                       |
|                          +---------------------------------+
|                          |   `TLS`_                        |
|                          +---------------------------------+
|                          |   :ref:`FOTA <nrf9160_ug_fota>` |
+--------------------------+---------------------------------+
| `Azure IoT Hub`_         |   `MQTT`_                       |
|                          +---------------------------------+
|                          |   `TLS`_                        |
|                          +---------------------------------+
|                          |   :ref:`FOTA <nrf9160_ug_fota>` |
+--------------------------+---------------------------------+
| `nRF Cloud`_             |   `MQTT`_                       |
|                          +---------------------------------+
|                          |   `TLS`_                        |
|                          +---------------------------------+
|                          |   :ref:`FOTA <nrf9160_ug_fota>` |
+                          +---------------------------------+
|                          |   :ref:`lib_nrf_cloud_agps`     |
+--------------------------+---------------------------------+
|                          |   :ref:`lib_nrf_cloud_pgps`     |
+--------------------------+---------------------------------+

By default, the application is configured to communicate with `nRF Cloud`_ using the factory-provisioned certificates on Thingy:91 and nRF9160 DK.
This enables the application to function out-of-the-box with nRF Cloud.
However, nRF Cloud does not fully support the application firmware and has limitations.
For more information, see :ref:`nRF Cloud implementation limitations <nrf_cloud_limitations>`.
To enable all features of the Asset Tracker v2, use the other supported cloud service implementations.

.. note::
   The Azure FOTA process is expected to change in the near future depending on the new `Azure Device Update for IoT Hub`_ that is currently in preview.

Setting up the Asset Tracker cloud example
------------------------------------------

To set up the application to work with a specific cloud example, see the following documentation:

* nRF Cloud - `Creating an nRF Cloud account`_ and `Connecting your device to nRF Cloud`_.
* AWS IoT Core - `Getting started guide for nRF Asset Tracker for AWS`_
* Azure IoT Hub - `Getting started guide for nRF Asset Tracker for Azure`_

For every cloud service that is supported by this application, you must configure the corresponding *cloud library* by setting certain mandatory Kconfig options that are specific to the cloud library.
For more information, see :ref:`Cloud-specific mandatory Kconfig options <mandatory_config>`.

Configuration options
=====================

Check and configure the following configuration options for the application:

.. option:: CONFIG_ASSET_TRACKER_V2_APP_VERSION - Configuration for providing the application version

   The application publishes its version number as a part of the static device data. The default value for the application version is ``0.0.0-development``. To configure the application version, set :option:`CONFIG_ASSET_TRACKER_V2_APP_VERSION` to the desired version.

.. option:: CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM - Configuration for enabling the use of custom cloud client ID

   This application configuration is used to enable the use of custom client ID for the respective cloud. By default, the application uses the IMEI of the nRF9160-based device as the client ID in the cloud connection.

.. option:: CLOUD_CLIENT_ID - Configuration for providing a custom cloud client ID

   This application configuration sets a custom client ID for the respective cloud. For setting a custom client ID, you need to set :option:`CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM` to ``y``.


.. _default_config_values:

The default values for the device configuration parameters can be set by manipulating the following configurations:

.. option:: CONFIG_DATA_DEVICE_MODE - Configuration for the device mode

   This application configuration sets the device mode.

.. option:: CONFIG_DATA_ACTIVE_TIMEOUT_SECONDS - Configuration for Active mode

   This application configuration sets the Active mode timeout value.

.. option:: CONFIG_DATA_MOVEMENT_RESOLUTION_SECONDS - Configuration for Movement resolution

   This configuration sets the Movement resolution timeout value.

.. option:: CONFIG_DATA_MOVEMENT_TIMEOUT_SECONDS - Configuration for Movement timeout

   This configuration sets the Movement timeout value.

.. option:: CONFIG_DATA_ACCELEROMETER_THRESHOLD - Configuration for Accelerometer threshold

   This configuration sets the Accelerometer threshold value.

.. option:: CONFIG_DATA_GPS_TIMEOUT_SECONDS - Configuration for GPS timeout

   This configuration sets the GPS timeout value.


.. _mandatory_config:

Mandatory library configuration
===============================

You can set the mandatory library-specific Kconfig options in the :file:`prj.conf` file of the application.

Configurations for AWS IoT library
----------------------------------

* :option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :option:`CONFIG_AWS_IOT_SEC_TAG`


Configurations for Azure IoT Hub library
----------------------------------------

* :option:`CONFIG_AZURE_IOT_HUB_DPS_HOSTNAME`
* :option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE`
* :option:`CONFIG_AZURE_IOT_HUB_SEC_TAG`
* :option:`CONFIG_AZURE_FOTA_SEC_TAG`

.. note:
   The nRF Cloud library does not require any library-specific Kconfig options to be set.

Optional library configurations
===============================

You can add the following optional configurations to configure the heap or to provide additional information such as APN to the modem for registering with an LTE network:

* :option:`CONFIG_HEAP_MEM_POOL_SIZE` - Configures the size of the heap that is used by the application when encoding and sending data to the cloud. More information can be found in :ref:`memory_allocation`.
* :option:`CONFIG_PDN_DEFAULTS_OVERRIDE` - Used for manual configuration of the APN. Set the option to ``y`` to override the default PDP context configuration.
* :option:`CONFIG_PDN_DEFAULT_APN` - Used for manual configuration of the APN. An example is ``apn.example.com``.

The application supports Assisted GPS.
To set the source of the A-GPS data, set the following options:

* :option:`CONFIG_AGPS_SRC_SUPL` - Sets the external SUPL Client library as A-GPS data source. See the documentation on :ref:`supl_client_lib`.
* :option:`CONFIG_AGPS_SRC_NRF_CLOUD` - Sets nRF Cloud as A-GPS data source. You must set nRF Cloud as the firmware cloud backend.

Configuration files
===================

The application provides predefined configuration files for typical use cases.
You can find the configuration files in the :file:`applications/asset_tracker_v2/` directory.

It is possible to build the application with overlay files for both DTS and Kconfig to override the default values for the board.
The application contains examples of Kconfig overlays.

The following configuration files are available in the application folder:

* :file:`prj.conf` - Configuration file common for all build targets
* :file:`boards/thingy91_nrf9160ns.conf` - Configuration file specific for Thingy:91. The file is automatically merged with :file:`prj.conf` when you build for the ``thingy91_nrf9160ns`` build target.
* :file:`overlay-low-power.conf` - Configuration file that achieves the lowest power consumption by disabling features  that consume extra power like LED control and logging.
* :file:`overlay-debug.conf` - Configuration file that adds additional verbose logging capabilities to the application

Generally, Kconfig overlays have an ``overlay-`` prefix and a ``.conf`` extension.
Board-specific configuration files are placed in the :file:`boards` folder and are named as :file:`<BOARD>.conf`.
DTS overlay files are named the same as the build target and use the file extension ``.overlay``.
When the DTS overlay filename matches the build target, the overlay is automatically chosen and applied by the build system.

Building and running
********************

Before building and running the firmware ensure that the cloud side is set up.
Also, the device must be provisioned and configured with the certificates according to the instructions for the respective cloud for the connection attempt to succeed.

.. note::

   This application supports :ref:`ug_bootloader`, which is disabled by default.
   To enable the immutable bootloader, set ``CONFIG_SECURE_BOOT=y``.


.. |sample path| replace:: :file:`applications/asset_tracker_v2`
.. include:: /includes/build_and_run_nrf9160.txt

.. external_antenna_note_start

.. note::
   For nRF9160 DK v0.15.0 and later, set the :option:`CONFIG_NRF9160_GPS_ANTENNA_EXTERNAL` option to ``y`` when building the application to achieve the best external antenna performance.

.. external_antenna_note_end

Building with overlays
======================

To build with Kconfig overlay, it must be based to the build system, as shown in the following example:

``west build -b nrf9160dk_nrf9160ns -- -DOVERLAY_CONFIG=overlay-low-power.conf``

The above command will build for nRF9160 DK using the configurations found in :file:`overlay-low-power.conf`, in addition to the configurations found in :file:`prj_nrf9160dk_nrf9160ns.conf`.
If some options are defined in both files, the options set in the overlay take precedence.

Testing
=======

After programming the application and all the prerequisites to your development kit, test the application by performing the following steps:

1. |connect_kit|
#. Connect to the kit with a terminal emulator (for example, LTE Link Monitor). See :ref:`lte_connect` for more information.
#. Reset the development kit.
#. Observe in the terminal window that the development kit starts up in the Secure Partition Manager and that the application starts.
   This is indicated by the following output::

      *** Booting Zephyr OS build v2.4.0-ncs1-2616-g3420cde0e37b  ***
      <inf> event_manager: APP_EVT_START

#. Observe in the terminal window that LTE connection is established, indicated by the following output::

     <inf> event_manager: MODEM_EVT_LTE_CONNECTING
     ...
     <inf> event_manager: MODEM_EVT_LTE_CONNECTED

#. Observe that the device establishes connection to the cloud::

    <inf> event_manager: CLOUD_EVT_CONNECTING
    ...
    <inf> event_manager: CLOUD_EVT_CONNECTED

#. Observe that data is sampled periodically and sent to the cloud::

    <inf> event_manager: APP_EVT_DATA_GET_ALL
    <inf> event_manager: APP_EVT_DATA_GET - Requested data types (MOD_DYN, BAT, ENV, GNSS)
    <inf> event_manager: GPS_EVT_ACTIVE
    <inf> event_manager: SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED
    <inf> event_manager: MODEM_EVT_MODEM_DYNAMIC_DATA_READY
    <inf> event_manager: MODEM_EVT_BATTERY_DATA_READY
    <inf> event_manager: GPS_EVT_DATA_READY
    <inf> event_manager: DATA_EVT_DATA_READY
    <inf> event_manager: GPS_EVT_INACTIVE
    <inf> event_manager: DATA_EVT_DATA_SEND
    <wrn> data_module: No batch data to encode, ringbuffers empty
    <inf> event_manager: CLOUD_EVT_DATA_ACK

Known issues and limitations
****************************

.. _nrf_cloud_limitations:

Enabling full support for nRF Cloud is currently a work in progress.
Following are the current limitations in the nRF Cloud implementation of the Asset Tracker v2:

* Data that is sampled by the device must ideally be addressed to the cloud-side device state and published in a single packet for regular device updates.
  This is to avoid the unnecessary stack overhead associated with splitting the payload and the additional current consumption that might result from splitting and sending the data as separate packets.
  However, in the case of nRF Cloud implementation, the nRF Cloud front end supports only the display of APP_DATA_MODEM_DYNAMIC (networkInfo) and APP_DATA_MODEM_STATIC (deviceInfo) through the device shadow.
  The other supported data types (GPS, temperature, and humidity) must be sent in a specified format to a separate message endpoint for the front end to graphically represent the data.
  You can find the JSON protocol definitions for data sent to the message endpoint in `nRF Cloud JSON protocol schemas`_.

* The nRF Cloud web application does not support the manipulation of real-time configurations.
  However, this is possible by using the REST API calls described in `nRF Cloud Patch Device State`_.
  To manipulate the device configuration, the ``desired`` section of the device state must be populated with the desired configuration of the device.
  The following schema sets the various device configuration parameters to their default values:

   .. parsed-literal::
      :class: highlight

	{
		"desired":{
			"config":{
				"activeMode":true,
				"activeWaitTime":120,
				"movementTimeout":3600,
				"movementResolution":120,
				"gpsTimeout":60,
				"movementThreshold":10
			}
		}
	}

* nRF Cloud does not support a separate endpoint for *batch* data updates. To temporarily circumvent this, batched data updates are sent to the message endpoint.


Dependencies
************

This application uses the following |NCS| libraries and drivers:

* :ref:`event_manager`
* :ref:`lib_aws_iot`
* :ref:`lib_aws_fota`
* :ref:`lib_azure_iot_hub`
* :ref:`lib_azure_fota`
* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_fota`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_date_time`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`
* :ref:`lib_download_client`
* :ref:`lib_fota_download`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`

.. _asset_tracker_v2_internal_modules:

Internal modules
****************

The application has two types of modules:

* Module with dedicated thread
* Module without thread

Every module has an event manager handler function, which subscribes to one or more event types.
When an event is sent from a module, all subscribers receive that event in the respective handler, and acts on the event in the following ways:

1. The event is converted into a message
#. The event is either processed directly or queued.

Modules may also receive events from other sources such as drivers and libraries.
For instance, the cloud module will also receive events from the configured cloud backend.
The event handler converts the events to messages.
The messages are then queued in the case of the cloud module or processed directly in the case of modules that do not have a processing thread.

.. figure:: /images/asset_tracker_v2_module_structure.svg
    :alt: Event handling in modules

    Event handling in modules

Thread usage
============

In addition to system threads, some modules have dedicated threads to process messages.
Some modules receive messages that may potentially take an extended amount of time to process.
Such messages are not suitable to be processed directly in the event handler callbacks that run on the system queue.
Therefore, these modules have a dedicated thread to process the messages.

Application-specific threads:

* Main thread (app module)
* Data management module
* Cloud module
* Sensor module
* Modem module

Modules that do not have dedicated threads process events in the context of system work queue in the event manager callback.
Therefore, their workloads must be light and non-blocking.

All module threads have the following identical properties by default:

* Thread is initialized at boot.
* Thread is preemptive.
* Priority is set to the lowest application priority in the system, which is done by setting ``CONFIG_NUM_PREEMPT_PRIORITIES`` to ``1``.
* Thread is started instantly after it is initialized in the boot sequence.

Following is the basic structure for all the threads:

.. code-block:: c

   static void module_thread_fn(void)
   {
           struct module_specific msg;

           self.thread_id = k_current_get();
           module_start(&self);

           /* Module specific setup */

           state_set(STATE_DISCONNECTED);

           while (true) {
                   module_get_next_msg(&self, &msg);

                   switch (state) {
                   case STATE_DISCONNECTED:
                           on_state_disconnected(&msg);
                           break;
                   case STATE_CONNECTED:
                           on_state_connected(&msg);
                           break;
                   default:
                           LOG_WRN("Unknown state");
                           break;
                   }

                   on_all_states(&msg);
           }
   }

.. _memory_allocation:

Memory allocation
=================

Mostly, the modules use statically allocated memory.
Following are some features that rely on dynamically allocated memory, using the :ref:`Zephyr heap memory pool implementation <zephyr:heap_v2>`:

* Event manager events
* Encoding of the data that will be sent to cloud

You can configure the heap memory by using the :option:`CONFIG_HEAP_MEM_POOL_SIZE`.
The data management module that encodes data destined for cloud is the biggest consumer of heap memory.
Therefore, when adjusting buffer sizes in the data management module, you must also adjust the heap accordingly.
This avoids the problem of running out of heap memory in worst-case scenarios.
