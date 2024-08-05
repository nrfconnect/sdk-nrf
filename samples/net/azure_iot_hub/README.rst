.. _azure_iot_hub:

Azure IoT Hub
#############

.. contents::
   :local:
   :depth: 2

The Azure IoT Hub sample shows the communication of an Internet-connected device with an `Azure IoT Hub`_ instance.
This sample uses the :ref:`lib_azure_iot_hub` library to communicate with the IoT hub and the :ref:`lib_azure_fota` library to provide firmware over-the-air (FOTA) functionality.

.. |wifi| replace:: Wi-Fi®

.. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample supports the direct connection of an IoT device that is already registered to an Azure IoT Hub instance.
Alternatively, it supports the provisioning of the device using `Azure IoT Hub Device Provisioning Service (DPS)`_ to an IoT hub.
See the documentation on :ref:`lib_azure_iot_hub` library for more information.

The sample periodically publishes telemetry messages (events) to the connected Azure IoT Hub instance.
By default, telemetry messages are sent every 20 seconds.
To configure the default interval, set the device twin property ``desired.telemetryInterval``, which will be interpreted by the sample as seconds.
Here is an example of a telemetry message:

.. parsed-literal::
   :class: highlight

   {
     "temperature": 25.2,
     "timestamp": 151325
   }

In a telemetry message, ``temperature`` is a value between ``25.0`` and ``25.9``, and ``timestamp`` is the uptime of the device in milliseconds.

The sample has implemented the handling of `Azure IoT Hub direct method`_ with the name ``led``.
If the device receives a direct method invocation with the name ``led`` and payload ``1`` or ``0``, LED 1 on the device is turned on or off, depending on the payload.
On Thingy:91, the LED turns red if the payload is ``1``.

Firmware update
***************

The sample subscribes to incoming `Azure IOT Hub device twin messages`_ that contain the firmware update information.
By default, the device twin document is requested upon connecting to the IoT hub.
Thus, any existing firmware information in the *desired* property will be parsed.
See :ref:`lib_azure_fota` for more details on the content of the firmware information in the device twin.

.. note::
   This sample requires a file server instance that hosts the new firmware image.
   The :ref:`lib_azure_fota` library does not require a specific host, but it has been tested using `Azure Blob Storage`_.

Configuration
*************

|config|

Setup
=====

For the sample to work as intended, you must set up and configure an Azure IoT Hub instance.
See :ref:`prereq_connect_to_azure_iot_hub` for information on creating an Azure IoT Hub instance and :ref:`configure_options_azure_iot` for additional information on the configuration options that are available.
Also, for a successful TLS connection to the Azure IoT Hub instance, the device needs to have credentials provisioned.
If you want to test FOTA, ensure that also the required credentials for the file server are provisioned and the :kconfig:option:`CONFIG_AZURE_FOTA_SEC_TAG` Kconfig option is set accordingly.
See :ref:`prereq_connect_to_azure_iot_hub` for information on provisioning the credentials.

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

.. _configure_options_azure_iot:

Additional configuration
========================

Check and configure the following library options that are used by the sample:

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` - Sets the Azure IoT Hub device ID. Alternatively, the device ID can be provided at run time.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` - Sets the Azure IoT Hub host name. If DPS is used, the sample assumes that the IoT hub host name is unknown, and the configuration is ignored. The configuration can also be omitted and the hostname provided at run time.

If DPS is used, use the Kconfig fragment found in the :file:`overlay-dps.conf` file and change the desired configurations there.
As an example, the following compiles with DPS for the nRF9160 DK:

.. code-block:: console

	west build -p -b nrf9160dk/nrf9160/ns -- -DEXTRA_CONF_FILE=overlay-dps.conf

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS` - Enables Azure IoT Hub DPS.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_REG_ID` - Sets the Azure IoT Hub DPS registration ID. It can be provided at run time. By default, the sample uses the device ID as the registration ID and sets it at run time.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE` - Sets the DPS ID scope of the Azure IoT Hub. This can be provided at run time.

For FOTA, check and configure the following library Kconfig options:

* :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION` - Defines the application version string. Indicates the current firmware version on the development kit.
* :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION_AUTO` - Automatically generates the application version. If enabled, :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION` is ignored.
* :kconfig:option:`CONFIG_AZURE_FOTA_TLS` - Enables HTTPS for downloads. By default, TLS is enabled and currently, the transport protocol must be configured at compile time.
* :kconfig:option:`CONFIG_AZURE_FOTA_SEC_TAG` - Sets the security tag for TLS credentials when using HTTPS as the transport layer. See :ref:`prereq_connect_to_azure_iot_hub` for more details.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` - Sets the Azure IoT Hub host name. If DPS is used, the sample assumes that the IoT hub host name is unknown, and the configuration is ignored.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` - Specifies the device ID, which is used when connecting to Azure IoT Hub.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_REG_ID` - Specifies the device registration ID used for DPS.

.. note::

   The sample sets the option :kconfig:option:`CONFIG_MQTT_KEEPALIVE` to the maximum allowed value, 1767 seconds (29.45 minutes) as specified by Azure IoT Hub.
   This is to limit the IP traffic between the device and the Azure IoT Hub message broker for supporting a low power sample.
   In certain LTE networks, the NAT timeout can be considerably lower than 1767 seconds.
   As a recommendation, and to prevent the likelihood of getting disconnected unexpectedly, set the option :kconfig:option:`CONFIG_MQTT_KEEPALIVE` to the lowest timeout limit (Maximum allowed MQTT keepalive and NAT timeout).

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/net/azure_iot_hub`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

Microsoft has created `Azure IoT Explorer`_ to interact and test devices connected to an Azure IoT Hub instance.
Optionally, follow the instructions at `Azure IoT Explorer`_ to install and configure the tool and use it as mentioned in the below instructions.

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe the log output and verify that it is similar to the :ref:`sampoutput_azure_iot`.
#. Use the `Azure IoT Explorer`_, or log in to the `Azure Portal`_.
#. Select the connected IoT hub and then your device.
#. Change the device twin's *desired* property ``telemetryInterval`` to a new value, for instance ``60``, and save the updated device twin.
   If it does not exist, you can add the *desired* property.
#. Observe that the device receives the updated ``telemetryInterval`` value, applies it, and starts sending new telemetry events every 10 seconds.
#. Verify that the ``reported`` object in the device twin now has a ``telemetryInterval`` property with the correct value reported back from the device.
#. In the `Azure IoT Explorer`_ or device page in `Azure Portal`_, navigate to the :guilabel:`Direct method` tab.
#. Enter ``led`` as the method name. In the **payload** field, enter the value ``1`` (or ``0``) and click :guilabel:`Invoke method`.
#. Observe that **LED 1** on the development kit lights up (or switches off if ``0`` is entered as the payload).
   If you are using `Azure IoT Explorer`_, you can observe a notification in the top right corner stating if the direct method was successfully invoked based on the report received from the device.
#. In the `Azure IoT Explorer`_, navigate to the :guilabel:`Telemetry` tab and click :guilabel:`start`.
#. Observe that the event messages from the device are displayed in the terminal within the specified telemetry interval.

To test FOTA update:

1. Navigate to the view where you can update the device twin using the `Azure IoT Explorer`_ or `Azure Portal`_.
#. Create a ``firmware`` JSON object inside the *desired* object of the device twin document, containing information about the binary to download:

   .. parsed-literal::
      :class: highlight

	{
	    "firmware": {
	        "fwVersion": "v0.0.2-dev",
	        "jobId": "ca186d4b-4171-4209-a49d-700c35567d1d",
	        "fwLocation": {
	            "host": "example.com",
	            "path": "firmware/my-app-update.bin"
	        },
	        "fwFragmentSize": 1800
	    }
	}

#. Click :guilabel:`Save` to apply the changes to the device twin.
   Updating the device twin causes a message to be sent to the device, containing the updates to the *desired* object.
#. In the terminal emulator, observe that the new firmware image is downloaded and installed as shown in :ref:`sampoutput_azure_iot`.
#. Observe that the sample displays the logs from the new application when the development kit reboots.
#. Confirm that the device twin contains the updated application version in the ``reported.firmware.fwVersion`` field in `Azure IoT Explorer`_ or `Azure Portal`_.

.. _sampoutput_azure_iot:

Sample output
=============

When the sample runs, the device boots, and the sample displays the following output in the terminal over UART:

.. code-block:: console

	*** Booting Zephyr OS build v2.3.0-rc1-ncs1-1453-gf41496cd30d5  ***
	<inf> azure_iot_hub_sample: Azure IoT Hub sample started
	<inf> azure_iot_hub_sample: Bringing network interface up and connecting to the network
	<inf> azure_iot_hub_sample: Device ID: my-device
	<inf> azure_iot_hub_sample: Connecting...
	<inf> azure_iot_hub_sample: Network connectivity established and IP address assigned
	<inf> azure_fota: Current firmware version: 0.0.1-dev
	<inf> azure_iot_hub_sample: Azure IoT Hub library initialized
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_CONNECTING
	<inf> azure_iot_hub_sample: Connection request sent to IoT Hub
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_CONNECTED
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_READY
	<inf> azure_iot_hub_sample: Sending event:
	<inf> azure_iot_hub_sample: {"temperature":25.9,"timestamp":16849}
	<inf> azure_iot_hub_sample: Event was successfully sent
	<inf> azure_iot_hub_sample: Next event will be sent in 60 seconds


If a new telemetry interval is set in the device twin, the console output is like this:

.. code-block:: console

	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED
	<inf> azure_iot_hub_sample: New telemetry interval has been applied: 60
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 42740
	<inf> azure_iot_hub_sample: Sending event:
	<inf> azure_iot_hub_sample: {"temperature":25.5,"timestamp":47585}
	<inf> azure_iot_hub_sample: Event was successfully sent
	<inf> azure_iot_hub_sample: Next event will be sent in 60 seconds

If a new FOTA update is initiated, the console output is like this:

.. code-block:: console

	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 140
	<inf> azure_fota: Attempting to download firmware (version 'v0.0.2-dev') from example.com/firmware/app_update.bin
	<inf> download_client: Downloading: firmware/app_update.bin [0]
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_FOTA_START
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED
	<inf> download_client: Setting up TLS credentials, sec tag count 1
	<inf> download_client: Connecting to example.com
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 190
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 190
	<inf> download_client: Downloaded 1800/674416 bytes (0%)
	...
	<inf> download_client: Downloaded 674416/674416 bytes (100%)
	<inf> download_client: Download complete
	<inf> dfu_target_mcuboot: MCUBoot image-0 upgrade scheduled. Reset device to apply
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_FOTA_DONE
	<inf> azure_iot_hub_sample: The device will reboot in 5 seconds to apply update
	<inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 109



Dependencies
************

The sample uses the following |NCS| and Zephyr libraries:

* :ref:`lib_azure_iot_hub`
* :ref:`lib_azure_fota`
* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`

It uses the following libraries and secure firmware component for nRF91 Series builds:

* :ref:`Trusted Firmware-M <ug_tfm>`
* :ref:`modem_info_readme`

It uses the following libraries for nRF7 Series builds:

* :ref:`nrf_security`
* :ref:`lib_wifi_credentials`
