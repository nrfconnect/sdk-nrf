.. _asset_tracker_v2_description:

Application description
#######################

.. contents::
   :local:
   :depth: 2

The Asset Tracker v2 application is built on the following principles:

* Ultra-low power by design - The application highlights the power saving features of the nRF9160 SiP, which is critical for successfully developing small form-factor devices and products which need very long battery lifetime.
* Offline first - Highly-mobile cellular IoT products need to handle unreliable connections gracefully by implementing mechanisms to retry the failed sending of data.
* Timestamping on the device - Sensor data is timestamped on the device using multiple time sources. When the device is offline (planned or unplanned), the timestamping does not rely on the cloud side.
* Batching of data - Data is batched to reduce the number of messages transmitted, and to be able to retain collected data while the device is offline.
* Configurable at run time - The application behavior (for example, accelerometer sensitivity or GNSS timeout) can be configured at run time. This improves the development experience with individual devices or when debugging the device behavior in specific areas and situations. It also reduces the cost for transmitting data to the devices by reducing the frequency of sending firmware updates to the devices.

.. note::
    The code is under active development. It will undergo changes and improvements in the future.

Overview
********

The application samples sensor data and publishes the data to a connected cloud service over `IP`_ through `LTE`_.
The application supports the following cloud services and corresponding cloud-side instances:

+-----------------------------------------------------------------------------------------------------------+--------------------------------+
| Cloud service                                                                                             | Cloud-side instance            |
+===========================================================================================================+================================+
| `AWS IoT Core`_                                                                                           | `nRF Asset Tracker for AWS`_   |
+-----------------------------------------------------------------------------------------------------------+--------------------------------+
| `Azure IoT Hub`_                                                                                          | `nRF Asset Tracker for Azure`_ |
+-----------------------------------------------------------------------------------------------------------+--------------------------------+
| `nRF Cloud`_                                                                                              | `nRF Cloud documentation`_     |
+-----------------------------------------------------------------------------------------------------------+--------------------------------+
| `LwM2M`_ v1.1 compliant service (`Coiote Device Management`_, `Leshan LwM2M server <Leshan homepage_>`_)  | Not yet implemented            |
+-----------------------------------------------------------------------------------------------------------+--------------------------------+

For more information on the cloud services, protocols, and technologies supported by the application, see the :ref:`Supported cloud services <supported_cloud_services>` table.

Firmware architecture
=====================

The Asset Tracker v2 application has a modular structure, where each module has a defined scope of responsibility.
The application makes use of the :ref:`app_event_manager` to distribute events between modules in the system.
The Application Event Manager is used for all the communication between the modules.
A module converts incoming events to messages and processes them in a FIFO manner.
The processing happens either in a dedicated processing thread in the module, or directly in the Application Event Manager callback.

The following figure shows the relationship between the modules and the Application Event Manager.
It also shows the modules with thread and the modules without thread.

.. figure:: /images/asset_tracker_v2_module_hierarchy.svg
    :alt: Module hierarchy

    Relationship between modules and the Application Event Manager

See :ref:`asset_tracker_v2_internal_modules` for more information.

Data types
==========

Data from multiple sensor sources are collected to construct information about the location, environment, and the health of the nRF9160-based device.
The application supports the following data types:

.. _app_data_types:

+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Data type      | Description                | Identifiers                                   | String identifier for NOD list |
+================+============================+===============================================+================================+
| Location       | GNSS coordinates           | APP_DATA_GNSS                                 |``gnss``                        |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Environmental  | Temperature, humidity      | APP_DATA_ENVIRONMENTAL                        | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Movement       | Acceleration               | APP_DATA_MOVEMENT                             | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Modem          | LTE link data, device data | APP_DATA_MODEM_DYNAMIC, APP_DATA_MODEM_STATIC | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Battery        | Voltage                    | APP_DATA_BATTERY                              | NA                             |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+
| Neighbor cells | Neighbor cell measurements | APP_DATA_NEIGHBOR_CELLS                       | ``ncell``                      |
+----------------+----------------------------+-----------------------------------------------+--------------------------------+

Sensor data published to the cloud service contain relative `timestamps <Timestamping_>`_ that originate from the time of sampling.

.. _real_time_configs:

Real-time configurations
========================

You can alter the core behavior of the application at run-time by updating the application's real-time configurations.
The real-time configurations supported by the application are listed in the following table:

+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Real-time Configurations       | Description                                                                                                                          | Default values |
+================================+======================================================================================================================================+================+
| Device mode                    | Either in active or passive mode.                                                                                                    | Active         |
+----------+---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|  Active  |                     | Sample and publish data at regular intervals.                                                                                        |                |
|          +---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Active wait time    | Number of seconds between each sampling/publication in active mode.                                                                  | 120 seconds    |
+----------+---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|  Passive |                     | Sample and publish data upon movement.                                                                                               |                |
|          +---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Movement resolution | Number of seconds between each sampling/publication in passive mode, given that the device is moving.                                | 120 seconds    |
|          +---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
|          | Movement timeout    | Number of seconds between each sampling/publication in passive mode, whether the device is moving or not.                            | 3600 seconds   |
+----------+---------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| GNSS timeout                   | Timeout for acquiring a GNSS fix during data sampling.                                                                               | 30 seconds     |
+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| Accelerometer threshold        | Accelerometer threshold in m/s². Minimal absolute value in m/s² for accelerometer readings to be considered valid movement.          | 10 m/s²        |
+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+
| No Data List (NOD)             | A list of strings that references :ref:`data types <app_data_types>`, which will not be sampled by the application.                  | No entries     |
|                                | Used to disable sampling from sensor sources.                                                                                        | (Request all)  |
|                                | For instance, when GNSS should be disabled in favor of location based on neighbor cell measurements,                                 |                |
|                                | the string identifier ``GNSS`` must be added to this list.                                                                           |                |
|                                | The supported string identifiers for each data type can be found in the :ref:`data types <app_data_types>` table.                    |                |
+--------------------------------+--------------------------------------------------------------------------------------------------------------------------------------+----------------+

You can alter the default values of the real-time configurations at compile time by setting the options listed in :ref:`Default device configuration options <default_config_values>`.

.. note::
   The application supports two different device modes, ``passive mode`` and ``active mode``.
   Depending on the device mode, specific configurations are used.
   For example, in active mode, ``Active wait time`` is used, but ``Movement resolution`` and ``Movement timeout`` are not.


The following flow charts show the operation of the application in the active and passive mode.
The charts also show the relationship between data sampling, publishing, and the real-time configurations.
All configurations that are not essential to this relationship are abstracted away.

.. figure:: /images/asset_tracker_v2_active_state.svg
    :alt: Active state flow chart

    Active mode flow chart

In the active mode, the application samples and publishes data at regular intervals that are set by the ``Active wait timeout`` configuration.

.. figure:: /images/asset_tracker_v2_passive_state.svg
    :alt: Passive mode flow chart

    Passive mode flow chart

In the passive mode, the application samples and publishes data upon two occurrences:

* When the timer controlled by the ``Movement resolution`` configuration expires and movement is detected.
* When the timer controlled by the ``Movement timeout`` configuration expires.
  This timer acts as failsafe if no movement is detected for extended periods of time.
  Essentially, it makes sure that the application always publishes data at some rate, regardless of movement.

.. note::
   The application receives its latest real-time configuration in one of two ways:

   * Upon every established connection to the configured cloud service.
   * When the device exits `Power Saving Mode (PSM)`_ to publish data.

   The application maintains its real-time configuration in the non-volatile flash memory.
   If the device unexpectedly reboots, the application still has access to the real-time configuration that was last applied.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm_spm_thingy91.txt

User interface
**************

The application supports basic UI elements to visualize its operating state and to notify the cloud using button presses.
This functionality is implemented in the :ref:`UI module <asset_tracker_v2_ui_module>` and the supported
LED patterns are documented in the :ref:`UI module LED indication <led_indication>` section.

.. _atv2_lwm2m_carrier_support:

Using the LwM2M carrier library
*******************************

The application supports the |NCS| :ref:`liblwm2m_carrier_readme` library that you can use to connect to the operator's device management platform.
See the library's documentation for more information and configuration options.

To enable the LwM2M carrier library, add the parameter ``-DOVERLAY_CONFIG=overlay-carrier.conf`` to your build command.

The CA root certificates that are needed for modem FOTA are not provisioned in the Asset Tracker v2 application.
You can flash the :ref:`lwm2m_carrier` sample to write the certificates to modem before flashing the Asset Tracker v2 application, or use the :ref:`at_client_sample` sample as explained in :ref:`Preparing the nRF9160: LwM2M Client sample for production <lwm2m_client_provisioning>`.
It is also possible to modify the Asset Tracker v2 project itself to include the certificate provisioning, as demonstrated in the :ref:`lwm2m_carrier` sample.

.. code-block:: c

   int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
   {
           switch (event->type) {
           case LWM2M_CARRIER_EVENT_INIT:
                   carrier_cert_provision();
           ...

A-GPS and P-GPS
***************

The application supports processing of incoming A-GPS and P-GPS data to reduce the GNSS Time-To-First-Fix (`TTFF`_).
Requesting and processing of A-GPS data is a default feature of the application.
See :ref:`nRF Cloud A-GPS and P-GPS <nrfcloud_agps_pgps>` for further details.
To enable support for P-GPS, add the parameter ``-DOVERLAY_CONFIG=overlay-pgps.conf`` to your build command.

.. note::
   |gps_tradeoffs|

Configuration
*************

The application has a Kconfig file with options that are specific to the Asset Tracker v2, where each of the modules has a separate submenu.
These options can be used to enable and disable modules and modify their behavior and properties.
|config|

Setting up the Asset Tracker cloud example
==========================================

To set up the application to work with a specific cloud example, see the following documentation:

* nRF Cloud - :ref:`Connecting your device to nRF Cloud <nrf9160_gs_connecting_dk_to_cloud>`.
* AWS IoT Core - `Getting started guide for nRF Asset Tracker for AWS`_.
* Azure IoT Hub - `Getting started guide for nRF Asset Tracker for Azure`_.

.. note::
   The `nRF Asset Tracker project`_ does not currently have an example implementation for LwM2M.

By default, the application is configured to communicate with `nRF Cloud`_ using the factory-provisioned certificates on Thingy:91 and nRF9160 DK.
This enables the application to function out-of-the-box with nRF Cloud.

.. note::
   Before building and running the firmware, make sure you have set up the cloud side and provisioned the device with the correct TLS certificates.

For every cloud service that is supported by this application, you must configure the corresponding cloud client library by setting certain mandatory Kconfig options.
You can set these options in the designated :file:`overlay-<feature>.conf` file located in the root folder of the application.
For more information on how to configure the application to communicate with a specific cloud service, see :ref:`Cloud module documentation <asset_tracker_v2_cloud_module>` and :ref:`Cloud-specific mandatory Kconfig options <mandatory_config>`.

Configuration options
=====================

Check and configure the following configuration options for the application:

.. _CONFIG_ASSET_TRACKER_V2_APP_VERSION:

CONFIG_ASSET_TRACKER_V2_APP_VERSION - Configuration for providing the application version
   The application publishes its version number as a part of the static device data. The default value for the application version is ``0.0.0-development``. To configure the application version, set :ref:`CONFIG_ASSET_TRACKER_V2_APP_VERSION <CONFIG_ASSET_TRACKER_V2_APP_VERSION>` to the desired version.

Optional library configurations
===============================

You can add the following optional configurations to configure the heap or to provide additional information such as APN to the modem for registering with an LTE network:

* :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - Configures the size of the heap that is used by the application when encoding and sending data to the cloud. More information can be found in :ref:`memory_allocation`.
* :kconfig:option:`CONFIG_PDN_DEFAULTS_OVERRIDE` - Used for manual configuration of the APN. Set the option to ``y`` to override the default PDP context configuration.
* :kconfig:option:`CONFIG_PDN_DEFAULT_APN` - Used for manual configuration of the APN. An example is ``apn.example.com``.

If you use an external GNSS antenna, add the following configuration:

* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna.

Configuration files
===================

The application provides predefined configuration files for typical use cases.
You can find the configuration files in the :file:`applications/asset_tracker_v2/` directory.

It is possible to build the application with overlay files for both DTS and Kconfig to override the default values for the board.
The application contains examples of Kconfig overlays.

The following configuration files are available in the application folder:

* :file:`prj.conf` - Configuration file common for ``thingy91_nrf9160_ns`` and ``nrf9160dk_nrf9160_ns`` build targets.
* :file:`prj_qemu_x86.conf` - Configuration file common for ``qemu_x86`` build target.
* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file specific for Thingy:91. This file is automatically merged with the :file:`prj.conf` file when you build for the ``thingy91_nrf9160_ns`` build target.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file specific for nRF9160 DK. This file is automatically merged with the :file:`prj.conf` file when you build for the ``nrf9160dk_nrf9160_ns`` build target.
* :file:`overlay-aws.conf` - Configuration file that enables communication with AWS IoT Core.
* :file:`overlay-azure.conf` - Configuration file that enables communication with Azure IoT Hub.
* :file:`overlay-lwm2m.conf` - Configuration file that enables communication with a configured LwM2M server.
* :file:`overlay-pgps.conf` - Configuration file that enables P-GPS.
* :file:`overlay-low-power.conf` - Configuration file that achieves the lowest power consumption by disabling features that consume extra power, such as LED control and logging.
* :file:`overlay-debug.conf` - Configuration file that adds additional verbose logging capabilities and enables the debug module.
* :file:`overlay-memfault.conf` - Configuration file that enables `Memfault`_.
* :file:`overlay-carrier.conf` - Configuration file that adds |NCS| :ref:`liblwm2m_carrier_readme` support. See :ref:`atv2_lwm2m_carrier_support` for more information.
* :file:`boards/<BOARD>/led_state_def.h` - Header file that describes the LED behavior of the CAF LEDs module.

Generally, Kconfig overlays have an ``overlay-`` prefix and a :file:`.conf` extension.
Board-specific configuration files are placed in the :file:`boards` folder and are named as :file:`<BOARD>.conf`.
DTS overlay files are named the same as the build target and use the file extension :file:`.overlay`.
They are placed in the :file:`boards` folder.
When the DTS overlay filename matches the build target, the overlay is automatically chosen and applied by the build system.
An overlay file typically enables a specific feature and can be included with other overlay files to enable multiple features at the same time.

Building and running
********************

To change the cloud service the application connects to, include the corresponding overlay file in the ``west build`` command.
See :ref:`Building with overlays <building_with_overlays>` for information on how to build with overlay files.

.. note::

   This application supports the :ref:`ug_bootloader` (also called immutable bootloader), which has been enabled by default since the |NCS| v2.0.0 release.
   Devices that do not already have the immutable bootloader cannot be firmware upgraded over the air to use the immutable bootloader.
   To disable the :ref:`ug_bootloader`, set both :kconfig:option:`CONFIG_SECURE_BOOT` and :kconfig:option:`CONFIG_BUILD_S1_VARIANT` Kconfig options to ``n``.


.. |sample path| replace:: :file:`applications/asset_tracker_v2`
.. include:: /includes/thingy91_build_and_run.txt

.. external_antenna_note_start

.. note::

   When you build the application for the nRF9160 DK v0.15.0 and later, set the :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` option to ``y`` to achieve the best external antenna performance.

.. external_antenna_note_end

.. _building_with_overlays:

Building with overlays
======================

To build with a Kconfig overlay, pass it to the build system using the ``OVERLAY_CONFIG`` CMake variable, as shown in the following example:

.. code-block:: console

   west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-low-power.conf

This command builds for the nRF9160 DK using the configurations found in the :file:`overlay-low-power.conf` file, in addition to the configurations found in the :file:`prj.conf` file.
If some options are defined in both files, the options set in the overlay take precedence.

To build with multiple overlay files, ``-DOVERLAY_CONFIG`` must be set to a list of overlay configurations, as shown in the following example:

.. code-block:: console

   west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG="overlay-aws.conf;overlay-debug.conf;overlay-memfault.conf"

Testing
=======

After programming the application and all the prerequisites to your development kit, test the application by performing the following steps:

1. |connect_kit|
#. Connect to the kit with a terminal emulator (for example, LTE Link Monitor). See :ref:`lte_connect` for more information.
#. Reset the development kit.
#. Observe in the terminal window that the development kit starts up in the Trusted Firmware-M secure firmware when building with nRF9160 DK or in the Secure Partition Manager when building with Thingy:91 and that the application starts.
   This is indicated by the following output::

      *** Booting Zephyr OS build v2.4.0-ncs1-2616-g3420cde0e37b  ***
      <inf> app_event_manager: APP_EVT_START

#. Observe in the terminal window that LTE connection is established, indicated by the following output::

     <inf> app_event_manager: MODEM_EVT_LTE_CONNECTING
     ...
     <inf> app_event_manager: MODEM_EVT_LTE_CONNECTED

#. Observe that the device establishes connection to the cloud::

    <inf> app_event_manager: CLOUD_EVT_CONNECTING
    ...
    <inf> app_event_manager: CLOUD_EVT_CONNECTED

#. Observe that data is sampled periodically and sent to the cloud::

   <inf> app_event_manager: APP_EVT_DATA_GET_ALL
   <inf> app_event_manager: APP_EVT_DATA_GET - Requested data types (MOD_DYN, BAT, ENV, GNSS)
   <inf> app_event_manager: GNSS_EVT_ACTIVE
   <inf> app_event_manager: SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED
   <inf> app_event_manager: MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY
   <inf> app_event_manager: MODEM_EVT_BATTERY_DATA_READY
   <inf> app_event_manager: GNSS_EVT_INACTIVE
   <inf> app_event_manager: GNSS_EVT_DATA_READY
   <inf> app_event_manager: DATA_EVT_DATA_READY
   <inf> app_event_manager: DATA_EVT_DATA_SEND_BATCH
   <inf> app_event_manager: CLOUD_EVT_DATA_SEND_QOS

.. _asset_tracker_v2_internal_modules:

Internal modules
****************

The application has two types of modules:

* Module with dedicated thread
* Module without thread

Every module has an Application Event Manager handler function, which subscribes to one or more event types.
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

For more information about each module and its configuration, see the :ref:`Subpages <asset_tracker_v2_subpages>`.

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

Modules that do not have dedicated threads process events in the context of system work queue in the Application Event Manager callback.
Therefore, their workloads must be light and non-blocking.

All module threads have the following identical properties by default:

* Thread is initialized at boot.
* Thread is preemptive.
* Priority is set to the lowest application priority in the system, which is done by setting :kconfig:option:`CONFIG_NUM_PREEMPT_PRIORITIES` to ``1``.
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

* Application Event Manager events
* Encoding of the data that will be sent to cloud

You can configure the heap memory by using the :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE`.
The data management module that encodes data destined for cloud is the biggest consumer of heap memory.
Therefore, when adjusting buffer sizes in the data management module, you must also adjust the heap accordingly.
This avoids the problem of running out of heap memory in worst-case scenarios.

Dependencies
************

This application uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`
* :ref:`lib_aws_iot`
* :ref:`lib_aws_fota`
* :ref:`lib_azure_iot_hub`
* :ref:`lib_azure_fota`
* :ref:`lwm2m_interface`
* :ref:`lib_lwm2m_client_utils`
* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_fota`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`lib_date_time`
* :ref:`lte_lc_readme`
* :ref:`modem_info_readme`
* :ref:`lib_download_client`
* :ref:`lib_fota_download`
* :ref:`caf_leds`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware components:

* :ref:`secure_partition_manager`
* :ref:`Trusted Firmware-M <ug_tfm>`
