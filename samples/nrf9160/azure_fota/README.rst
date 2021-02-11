.. _azure_fota_sample:

nRF9160: Azure FOTA
###################

.. contents::
   :local:
   :depth: 2

The Azure firmware over-the-air (Azure FOTA) sample demonstrates how to perform an over-the-air firmware update of an nRF9160-based device using the :ref:`lib_azure_fota` and :ref:`lib_azure_iot_hub` libraries of |NCS|.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns, nrf9160dk_nrf9160ns

The sample also requires an Azure IoT Hub instance, and optionally an `Azure IoT Hub Device Provisioning Service (DPS)`_ instance, if the device is not already registered with the IoT hub.

.. include:: /includes/spm.txt

Overview
********

The sample connects to the configured `Azure IoT Hub`_ and waits for incoming `Azure IOT Hub device twin messages`_ that contain the firmware update information.
By default, the device twin document is requested upon connecting to the IoT hub.
Thus, any existing firmware information in the *desired* property will be parsed.
See :ref:`lib_azure_fota` for more details on the content of the firmware information in the device twin.

.. note::
   This sample requires a file server instance that hosts the new firmware image.
   The :ref:`lib_azure_fota` library does not require a specific host, but it has been tested using `Azure Blob Storage`_ that shares the same root certificate as the Azure IoT Hub instance.

.. _certificates:

Certificates for using TLS
==========================

If TLS is used as the transport layer, the required certificates must be provisioned to the device.
The certificates that need to be provisioned depends on the location of the hosted FOTA image, and the TLS settings that are configured at the endpoint from where the file is downloaded.
If, for instance, Azure Blob Storage is used for hosting, the same root certificate (`Baltimore CyberTrust Root certificate`_) that is used in the `Azure IoT Hub`_ connection can be used.
See :ref:`prereq_connect_to_azure_iot_hub` for more information.

If a host other than Azure is used, the certificate requirements might be different.
See the documentation for the respective host to locate the correct certificates.
The certificates can be provisioned using the same procedure as described in :ref:`azure_iot_hub_flash_certs`.


.. _additional_config_azure_fota:

Additional configuration
========================

Check and configure the following library options that are used by the sample:

* :option:`CONFIG_AZURE_FOTA_APP_VERSION` - Defines the application version string. Indicates the current firmware version on the development kit.
* :option:`CONFIG_AZURE_FOTA_APP_VERSION_AUTO` - Automatically generates the application version. If enabled, :option:`CONFIG_AZURE_FOTA_APP_VERSION` is ignored.
* :option:`CONFIG_AZURE_FOTA_TLS` - Enables HTTPS for downloads. By default, TLS is enabled and currently, the transport protocol must be configured at compile time.
* :option:`CONFIG_AZURE_FOTA_SEC_TAG` - Sets the security tag for TLS credentials when using HTTPS as the transport layer. See :ref:`certificates` for more details.
* :option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` - Sets the Azure IoT Hub host name. If DPS is used, the sample assumes that the IoT hub host name is unknown, and the configuration is ignored.
* :option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` - Specifies the device ID, which is used when connecting to Azure IoT Hub or when DPS is enabled.

.. note::
   If the :option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID_APP` option is disabled, the device ID must be set in :file:`prj.conf`.
   If the :option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID_APP` option is enabled, the device ID must be provided using the :c:struct:`azure_iot_hub_config` configuration struct in a call to the :c:func:`azure_iot_hub_init` function.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/azure_fota`

.. include:: /includes/build_and_run_nrf9160.txt



Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe that the sample displays the configured application version in the terminal and connects to Azure IoT Hub as shown below::

      *** Booting Zephyr OS build v2.3.0-rc1-ncs1-1453-gf41496cd30d5  ***
      Traces enabled
      Azure FOTA sample started
      Initializing Modem library
      This may take a while if a modem firmware update is pending
      Modem library initialized
      Connecting to LTE network
      AZURE_IOT_HUB_EVT_CONNECTING
      AZURE_IOT_HUB_EVT_CONNECTED
      AZURE_IOT_HUB_EVT_READY


#. Log in to the `Azure Portal`_, navigate to :guilabel:`IoT Hub` and select your IoT hub.
#. Navigate to :guilabel:`IoT devices` and select the device to update.
#. In the device view, click the :guilabel:`Device Twin` button.
#. Create a `firmware` JSON object inside the *desired* object of the device twin document, containing information about the binary to download:

   .. parsed-literal::
      :class: highlight

	{
	    "firmware": {
	        "fwVersion": "v0.0.0-dev",
	        "jobId": "ca186d4b-4171-4209-a49d-700c35567d1d",
	        "fwLocation": {
	            "host": "my-storage-account.blob.core.windows.net",
	            "path": "my-app-update.bin"
	        },
	        "fwFragmentSize": 1800
	    }
	}

   See the documentation on :ref:`lib_azure_fota` library for more details.
#. Apply the changes to the device twin by clicking :guilabel:`Save`. Updating the device twin causes a message to be sent to the device, containing the updates to the *desired* object.
#. In the terminal emulator, observe that the new firmware image is downloaded and installed as shown below::

      ...
      [00:00:20.089,416] <inf> download_client: Downloaded 19800/300860 bytes (6%)
      [00:00:20.338,653] <inf> download_client: Downloaded 21600/300860 bytes (7%)
      [00:00:20.359,924] <inf> STREAM_FLASH: Erasing page at offset 0x0008c000
      [00:00:20.729,644] <inf> download_client: Downloaded 23400/300860 bytes (7%)
      [00:00:20.979,675] <inf> download_client: Downloaded 25200/300860 bytes (8%)
      [00:00:21.007,385] <inf> STREAM_FLASH: Erasing page at offset 0x0008d000
      ...
      [00:01:22.750,946] <inf> download_client: Download complete
      [00:01:22.761,657] <inf> STREAM_FLASH: Erasing page at offset 0x000fd000
      [00:01:22.857,238] <inf> dfu_target_mcuboot: MCUBoot image upgrade scheduled. Reset the device to apply
      [2020-08-28 00:38:18] [00:01:15.665,679] <inf> azure_fota: FOTA download completed evt received
      [2020-08-28 00:38:18] AZURE_IOT_HUB_EVT_FOTA_DONE
      [2020-08-28 00:38:18] The device will reboot in 5 seconds to apply update

#. When the development kit reboots, observe that the sample displays the logs from the new application.
#. In Azure Portal, confirm that the device twin contains the updated application version in the ``reported.firmware.fwVersion`` field.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_azure_fota`
* :ref:`lib_azure_iot_hub`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`MQTT <zephyr:networking_api>`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
