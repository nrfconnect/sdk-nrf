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

Requirements
************

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. include:: /includes/external_flash_nrf91.txt

Overview
********

The application samples and sends sensor data to a connected cloud service over `IP`_ using `LTE`_.
Following are the cloud services that are supported by the application:

* `AWS IoT Core`_
* `Azure IoT Hub`_
* `nRF Cloud`_
* `LwM2M`_ v1.1 compliant service (for example, AVSystem's `Coiote Device Management`_, `Leshan LwM2M server <Leshan homepage_>`_).
  To know more about the AVSystem integration with |NCS|, see :ref:`ug_avsystem`.

To run the application on a development kit and connect to a cloud service, you must complete the following steps:

1. :ref:`Setting up the cloud service <cloud_setup>`
#. :ref:`Configuring the application <atv2_application_configuration>`
#. :ref:`Building and running <building_and_running>`

.. note::
   For more information on the protocols and technologies that are used in the connection towards a specific cloud service, see :ref:`Supported cloud services <supported_cloud_services>`.

.. _cloud_setup:

Cloud setup
***********

To set up a cloud service to work with the application firmware, complete the steps:

.. important::
   The application defaults to the *client ID* used in the connection to the *IMEI* of the device.
   This value is printed on the development kit.

* nRF Cloud - :ref:`Connecting your device to nRF Cloud <nrf9160_gs_connecting_dk_to_cloud>`.
  The default configuration of the firmware is to communicate with `nRF Cloud`_ using the factory-provisioned certificates on the Thingy:91 and nRF9160 DK.
  This means that no additional configuration of the firmware is needed to connect to nRF Cloud.
  It is recommended to build and run the firmware on the device before completing the steps listed in :ref:`Connecting your device to nRF Cloud <nrf9160_gs_connecting_dk_to_cloud>`.
  See :ref:`Building and running <building_and_running>`.

* AWS IoT Core - :ref:`lib_aws_iot`.
  This step retrieves a *security tag* and a *hostname* that will be needed during the configuration of the firmware.

* Azure IoT Hub - :ref:`lib_azure_iot_hub`.
  This step retrieves a *security tag* and *ID scope* that will be needed during the configuration of the firmware.
  Make sure to follow the steps documented in the :ref:`dps_config` section to enable Device Provisioning Service (DPS).

* AVSystem's LwM2M Coiote Device Management - :ref:`server_setup_lwm2m_client`.
  No additional configuration is needed if the server is set up according to the linked documentation.

You can also use the nRF Asset Tracker project that provides an example cloud implementation for multiple cloud providers.
See the :ref:`nRF_asset_tracker` section for more details.

.. _atv2_lwm2m_carrier_support:

Carrier library support
***********************

See the section :ref:`using_the_lwm2m_carrier_library` for steps on how to connect to a operator's device management platform, necessary in some LTE networks.

.. _atv2_application_configuration:

Configuration
*************

To configure the firmware to connect to a specific cloud service, complete the following steps:

1. Update the overlay configuration file that corresponds to the selected cloud service with the Kconfig option values that were extracted during the :ref:`Setting up the cloud service <cloud_setup>` step.
   The name of the overlay file must reflect the cloud service that has been chosen.
   For example, the file name is :file:`overlay-aws.conf` for AWS IoT.
   :ref:`Cloud-specific mandatory Kconfig options <mandatory_config>` lists the mandatory options that are specific to each cloud service.
   The overlay files are located in the root folder of the application, under :file:`applications/asset_tracker_v2/`.

#. Include the overlay file when building the firmware, as documented in :ref:`Building and running <building_and_running>`.

Configuration files
===================

The application provides predefined configuration files for typical use cases.

Following are the available configuration files:

* :file:`prj.conf` - Configuration file common for ``thingy91_nrf9160_ns`` and ``nrf9160dk_nrf9160_ns`` build targets.
* :file:`prj_qemu_x86.conf` - Configuration file common for ``qemu_x86`` build target.
* :file:`prj_native_posix.conf` - Configuration file common for ``native_posix`` build target.
* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file specific for Thingy:91. This file is automatically merged with the :file:`prj.conf` file when you build for the ``thingy91_nrf9160_ns`` build target.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file specific for nRF9160 DK. This file is automatically merged with the :file:`prj.conf` file when you build for the ``nrf9160dk_nrf9160_ns`` build target.
* :file:`boards/<BOARD>/led_state_def.h` - Header file that describes the LED behavior of the CAF LEDs module.

Overlay configurations files that enable specific features:

* :file:`overlay-aws.conf` - Configuration file that enables communication with AWS IoT Core.
* :file:`overlay-azure.conf` - Configuration file that enables communication with Azure IoT Hub.
* :file:`overlay-lwm2m.conf` - Configuration file that enables communication with AVSystem's Coiote IoT Device Management.
* :file:`overlay-pgps.conf` - Configuration file that enables P-GPS.
* :file:`overlay-nrf7002ek-wifi-scan-only.conf` - Configuration file that enables Wi-Fi scanning with nRF7002 EK.
* :file:`overlay-low-power.conf` - Configuration file that achieves the lowest power consumption by disabling features that consume extra power, such as LED control and logging.
* :file:`overlay-debug.conf` - Configuration file that adds additional verbose logging capabilities and enables the debug module.
* :file:`overlay-memfault.conf` - Configuration file that enables `Memfault`_.
* :file:`overlay-carrier.conf` - Configuration file that adds |NCS| :ref:`liblwm2m_carrier_readme` support. See :ref:`atv2_lwm2m_carrier_support` for more information.
* :file:`overlay-full_modem_fota.conf` - Configuration file that enables full modem FOTA.

Custom DTC overlay file for enabling a specific feature:

* :file:`nrf9160dk_with_nrf7002ek.overlay` - Configuration file that enables Wi-Fi scanning with nRF7002 EK.

Multiple overlay files can be included to enable multiple features at the same time.

.. note::

   Generally, Kconfig overlays have an ``overlay-`` prefix and a :file:`.conf` extension.
   Board-specific configuration files are placed in the :file:`boards` folder and are named as :file:`<BOARD>.conf`.
   DTS overlay files are named the same as the build target and use the file extension :file:`.overlay`.
   When they are placed in the :file:`boards` folder and the DTS overlay filename matches the build target,
   the build system automatically selects and applies the overlay.
   You can give the custom DTS overlay files as a compiler option ``-DDTC_OVERLAY_FILE=<dtc_filename>.overlay``.

Optional library configurations
===============================

You can add the following optional configurations to configure the heap or to provide additional information such as Access Point Name (APN) to the modem for registering with an LTE network:

* :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - Configures the size of the heap that is used by the application when encoding and sending data to the cloud. More information can be found in :ref:`memory_allocation`.
* :kconfig:option:`CONFIG_PDN_DEFAULTS_OVERRIDE` - Used for manual configuration of the APN. Set the option to ``y`` to override the default PDP context configuration.
* :kconfig:option:`CONFIG_PDN_DEFAULT_APN` - Used for manual configuration of the APN. An example is ``apn.example.com``.
* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna. For nRF9160 DK v0.15.0 and later, it is recommended to set this option to achieve the best external antenna performance.

.. note::
   This application supports the :ref:`ug_bootloader` (also called immutable bootloader), which has been enabled by default since the |NCS| v2.0.0 release.
   Devices that do not have the immutable bootloader cannot be upgraded over the air to use the immutable bootloader.
   To disable the :ref:`ug_bootloader`, set both :kconfig:option:`CONFIG_SECURE_BOOT` and :kconfig:option:`CONFIG_BUILD_S1_VARIANT` Kconfig options to ``n``.

.. _building_and_running:

Building and running
********************

This application can be found under :file:`applications/asset_tracker_v2` in the |NCS| folder structure.
See :ref:`gs_programming` for information about how to build and program the application.

Testing
=======

After programming the application and all the prerequisites to your development kit, test the application by performing the following steps:

1. |connect_kit|
#. Connect to the kit with a terminal emulator (for example, LTE Link Monitor). See :ref:`lte_connect` for more information.
#. Reset the development kit.
#. Observe in the terminal window that application boots as shown in the following output::

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
   <inf> app_event_manager: APP_EVT_DATA_GET - Requested data types (MOD_DYN, BAT, ENV, LOCATION)
   <inf> app_event_manager: LOCATION_MODULE_EVT_ACTIVE
   <inf> app_event_manager: SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED
   <inf> app_event_manager: SENSOR_EVT_FUEL_GAUGE_NOT_SUPPORTED
   <inf> app_event_manager: MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY
   <inf> app_event_manager: LOCATION_MODULE_EVT_INACTIVE
   <inf> app_event_manager: LOCATION_MODULE_EVT_GNSS_DATA_READY
   <inf> app_event_manager: DATA_EVT_DATA_READY
   <inf> app_event_manager: DATA_EVT_DATA_SEND_BATCH
   <inf> app_event_manager: CLOUD_EVT_DATA_SEND_QOS


Dependencies
************

This application uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`
* :ref:`lib_aws_iot`
* :ref:`lib_aws_fota`
* :ref:`lib_azure_iot_hub`
* :ref:`lib_azure_fota`
* :ref:`lib_lwm2m_client_utils`
* :ref:`lib_lwm2m_location_assistance`
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

It uses the following Zephyr library:

* :ref:`lwm2m_interface`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

.. _nRF_asset_tracker:

nRF Asset Tracker project
*************************

The application firmware is a part of the `nRF Asset Tracker project`_, which is an open source end-to-end example that includes cloud backend and frontend code for the following cloud providers:

* AWS IoT Core - `Getting started guide for nRF Asset Tracker for AWS`_.
* Azure IoT Hub - `Getting started guide for nRF Asset Tracker for Azure`_.

The `nRF Asset Tracker project`_ contains separate documentation that goes through setup of the firmware and cloud-side setup.
If you want to use the nRF Asset Tracker, follow the `nRF Asset Tracker project`_ documentation instead of the Asset Tracker v2 documentation.
