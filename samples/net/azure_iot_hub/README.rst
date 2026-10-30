.. _azure_iot_hub:
.. _azure_iot_hub_sample:

.. ncs-sample::
   :title: Azure IoT Hub

   The Azure IoT Hub sample demonstrates the communication of an Internet-connected device with an `Azure IoT Hub`_ instance.
   This sample uses the :ref:`lib_azure_iot_hub` library to communicate with the IoT hub and the :ref:`lib_azure_fota` library to provide firmware over-the-air (FOTA) functionality.
   The sample connects either to an LTE network using an nRF91 Series device, or to Wi-Fi® using an nRF71 Series or nRF70 Series device combined with a compatible host platform.

   .. |wifi| replace:: Wi-Fi

   .. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, the sample supports emulation using the :zephyr:board:`native simulator <native_sim>`.

.. include:: /includes/tfm.txt

Overview
********

The sample supports the direct connection of an IoT device that is already registered to an Azure IoT Hub instance.
Alternatively, it supports the provisioning of the device using `Azure IoT Hub Device Provisioning Service (DPS)`_ to an IoT hub.
See the documentation on :ref:`lib_azure_iot_hub` library for more information.

The sample periodically publishes telemetry messages (events) to the connected Azure IoT Hub instance.
By default, telemetry messages are sent every 20 seconds.
To configure the default interval, set the device twin property ``desired.telemetryInterval``.
The sample reads the value of this property as seconds.
Here is an example of a telemetry message:

.. parsed-literal::
   :class: highlight

   {
     "temperature": 25.2,
     "timestamp": 151325
   }

In a telemetry message, ``temperature`` is a value between ``25.0`` and ``25.9``, and ``timestamp`` is the uptime of the device in milliseconds.

The sample handles the `Azure IoT Hub direct method`_ with the name ``led``.
If the device receives a direct method invocation with the name ``led`` and payload ``1`` or ``0``, LED 1 on the device is turned on or off, depending on the payload.

Firmware update
===============

The sample subscribes to incoming `Azure IOT Hub device twin messages`_ that contain the firmware update information.
By default, the sample requests the device twin document when the device connects to the IoT hub.
Thus, the sample parses any existing firmware information in the *desired* property.
See :ref:`lib_azure_fota` for more details on the content of the firmware information in the device twin.

.. note::
   This sample requires a file server instance that hosts the new firmware image.
   The :ref:`lib_azure_fota` library does not require a specific host, but it has been tested using `Azure Blob Storage`_.

Configuration
*************

|config|

Azure IoT Hub setup
===================

For the sample to work as intended, you must set up and configure an Azure IoT Hub instance.
See :ref:`prereq_connect_to_azure_iot_hub` for information on creating an Azure IoT Hub instance and :ref:`configure_options_azure_iot` for additional information on the configuration options that are available.
For a successful TLS connection to the Azure IoT Hub instance, you must provision credentials on the device.
If you want to test FOTA, ensure that you also provision the required credentials for the file server.
Also ensure that you set the :kconfig:option:`CONFIG_AZURE_FOTA_SEC_TAG` Kconfig option accordingly.
See :ref:`prereq_connect_to_azure_iot_hub` for information on provisioning the credentials.

.. note::
   On nRF91 Series devices, you can provision the credentials only when the modem is not connected to the network.

.. _configure_options_azure_iot:

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :ncs-file:`/samples/net/azure_iot_hub/Kconfig`):

.. options-from-kconfig::
   :show-type:

Check and configure the following library options that are used by the sample:

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DEVICE_ID` - Sets the Azure IoT Hub device ID.
  Alternatively, you can provide the device ID at runtime.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_HOSTNAME` - Sets the Azure IoT Hub host name.
  If you use the Device Provisioning Service (DPS), the sample assumes that the host name is unknown and ignores the configuration.
  You can also omit the configuration and provide the host name at run time.

If DPS is used, check and configure the following library options:

* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS` - Enables Azure IoT Hub DPS.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_REG_ID` - Sets the Azure IoT Hub DPS registration ID.
  You can provide it at runtime.
  By default, the sample uses the device ID as the registration ID and sets it at runtime.
* :kconfig:option:`CONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE` - Sets the DPS ID scope of the Azure IoT Hub.
  You can provide it at runtime.

For FOTA, check and configure the following library options:

* :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION` - Defines the application version string.
  Indicates the current firmware version on the development kit.
* :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION_AUTO` - Automatically generates the application version.
  If you enable this option, the sample ignores :kconfig:option:`CONFIG_AZURE_FOTA_APP_VERSION`.
* :kconfig:option:`CONFIG_AZURE_FOTA_TLS` - Enables HTTPS for downloads.
  By default, TLS is enabled.
  You must configure the transport protocol at compile time.
* :kconfig:option:`CONFIG_AZURE_FOTA_SEC_TAG` - Sets the security tag for TLS credentials when using HTTPS as the transport layer.
  See :ref:`prereq_connect_to_azure_iot_hub` for more details.

.. note::
   The sample sets the :kconfig:option:`CONFIG_MQTT_KEEPALIVE` Kconfig option to the maximum value that Azure IoT Hub allows, which is 1767 seconds (29.45 minutes).
   This value limits the IP traffic between the device and the Azure IoT Hub message broker, to support a low-power sample.
   In certain LTE networks, the NAT timeout can be considerably lower than 1767 seconds.
   To prevent unexpected disconnections, set the option :kconfig:option:`CONFIG_MQTT_KEEPALIVE` to the lowest timeout limit (maximum allowed MQTT keepalive and NAT timeout).

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* :ncs-file:`/samples/net/azure_iot_hub/prj.conf` - General configuration file for all devices.
* :ncs-file:`/samples/net/azure_iot_hub/boards/nrf9151dk_nrf9151_ns.conf` - Configuration file for the nRF9151 DK.
* :ncs-file:`/samples/net/azure_iot_hub/boards/nrf9161dk_nrf9161_ns.conf` - Configuration file for the nRF9161 DK.
* :ncs-file:`/samples/net/azure_iot_hub/boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :ncs-file:`/samples/net/azure_iot_hub/boards/native_sim.conf` - Configuration file for the native simulator emulation.
* :ncs-file:`/samples/net/azure_iot_hub/boards/nrf7120dk_nrf7120_cpuapp_ns.conf` - Configuration file for the nRF7120 DK.
* :ncs-file:`/samples/net/azure_iot_hub/boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - Configuration file for the nRF7002 DK.
* :ncs-file:`/samples/net/azure_iot_hub/boards/nrf54lm20dk_nrf54lm20a_cpuapp_ns.conf` - Configuration file for the nRF54LM20 DK (nRF54LM20A SoC).
* :ncs-file:`/samples/net/azure_iot_hub/boards/nrf54lm20dk_nrf54lm20b_cpuapp_ns.conf` - Configuration file for the nRF54LM20 DK (nRF54LM20B SoC).
* :ncs-file:`/samples/net/azure_iot_hub/wifi.conf` - Wi-Fi networking configuration, common to the nRF71 Series and nRF70 Series devices.
  Must be added explicitly for Wi-Fi builds.
* :file:`wifi-fota.conf` - FOTA configuration for Wi-Fi builds.
* :ncs-file:`/samples/net/azure_iot_hub/dps.conf` - Enables provisioning of the device using DPS.

Board files under the :file:`boards/` folder are merged automatically for the selected target.
The :ncs-file:`/samples/net/azure_iot_hub/wifi.conf` and other extra configuration files are not board-specific and must be passed with ``azure_iot_hub_EXTRA_CONF_FILE``.

To add a specific extra configuration file to the build, add the ``-- -Dazure_iot_hub_EXTRA_CONF_FILE=<extra_conf_file>`` flag to your west build command.

Wi-Fi
-----

Networking, TLS, and FOTA are not part of the default configuration when you build for Wi-Fi.
Add them with the :ncs-file:`/samples/net/azure_iot_hub/wifi.conf` and :file:`wifi-fota.conf` files.
For example, to build for the nRF7002 DK:

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp/ns -- -Dazure_iot_hub_EXTRA_CONF_FILE="wifi.conf;wifi-fota.conf"

To build for the nRF54LM20 DK with the nRF7002 EB2 shield, also add the shield:

.. code-block:: console

   west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp/ns -- -Dazure_iot_hub_SHIELD="nrf7002eb2" -Dazure_iot_hub_EXTRA_CONF_FILE="wifi.conf;wifi-fota.conf"

Device Provisioning Service
---------------------------

To use DPS, add the :ncs-file:`/samples/net/azure_iot_hub/dps.conf` file to the build.
For example, to build with DPS for the nRF9161 DK:

.. code-block:: console

   west build -p -b nrf9161dk/nrf9161/ns -- -Dazure_iot_hub_EXTRA_CONF_FILE=dps.conf

To use DPS on a Wi-Fi build, add it to the other files:

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp/ns -- -Dazure_iot_hub_EXTRA_CONF_FILE="wifi.conf;wifi-fota.conf;dps.conf"

Building and running
********************

.. |sample path| replace:: :file:`samples/net/azure_iot_hub`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

Microsoft has created `Azure IoT Explorer`_ to interact and test devices connected to an Azure IoT Hub instance.
Optionally, follow the instructions at `Azure IoT Explorer`_ to install and configure the tool and use it as described in the following instructions.

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset the kit.
#. Observe the log output and verify that it is similar to the :ref:`sampoutput_azure_iot`.
#. Use the `Azure IoT Explorer`_, or log in to the `Azure Portal`_.
#. Select the connected IoT hub and then your device.
#. Change the device twin's *desired* property ``telemetryInterval`` to a new value, for instance ``60``, and save the updated device twin.
   If it does not exist, you can add the *desired* property.
#. Observe that the device receives the updated ``telemetryInterval`` value, applies it, and starts sending new telemetry events every 60 seconds.
#. Verify that the ``reported`` object in the device twin now has a ``telemetryInterval`` property with the correct value reported back from the device.
#. In the `Azure IoT Explorer`_ or device page in `Azure Portal`_, navigate to the :guilabel:`Direct method` tab.
#. Enter ``led`` as the method name.
   In the **payload** field, enter the value ``1`` (or ``0``) and click :guilabel:`Invoke method`.
#. Observe that **LED 1** on the development kit lights up (or switches off if ``0`` is entered as the payload).
   If you are using `Azure IoT Explorer`_, you can observe a notification in the top right corner that shows if the direct method was successfully invoked, based on the report received from the device.
#. In the `Azure IoT Explorer`_, navigate to the :guilabel:`Telemetry` tab and click :guilabel:`start`.
#. Observe that the event messages from the device are displayed in the `Azure IoT Explorer`_ within the specified telemetry interval.

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
   <inf> downloader: Downloading: firmware/app_update.bin [0]
   <inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_FOTA_START
   <inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED
   <inf> downloader: Setting up TLS credentials, sec tag count 1
   <inf> downloader: Connecting to example.com
   <inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 190
   <inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 190
   <inf> downloader: Downloaded 1800/674416 bytes (0%)
   ...
   <inf> downloader: Downloaded 674416/674416 bytes (100%)
   <inf> downloader: Download complete
   <inf> dfu_target_mcuboot: MCUBoot image-0 upgrade scheduled. Reset device to apply
   <inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_FOTA_DONE
   <inf> azure_iot_hub_sample: The device will reboot in 5 seconds to apply update
   <inf> azure_iot_hub_sample: AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: 109

Troubleshooting
===============

If you have issues with connectivity on nRF91 Series devices, see the `Cellular Monitor app`_ documentation to learn how to capture modem traces to debug network traffic in Wireshark.
To enable modem traces, build the sample with the following command:

.. code-block:: console

   west build -p -b nrf9161dk/nrf9161/ns -- -Dazure_iot_hub_SNIPPET="nrf91-modem-trace-uart"

If you have issues with connectivity on nRF71 Series or nRF70 Series devices, see the :ref:`wifi_monitor_sample` sample documentation to learn how to capture and analyze Wi-Fi traffic to debug connectivity issues.
Also verify that the Wi-Fi credentials configured for your device match your access point, as described in the `Configuration options`_ section on this page.

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`lib_azure_iot_hub`
* :ref:`lib_azure_fota`
* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`

It uses the following libraries and secure firmware component for nRF91 Series builds:

* :ref:`Trusted Firmware-M <ug_tfm>`
* :ref:`modem_info_readme`

It uses the following libraries for nRF71 Series and nRF70 Series builds:

* :ref:`nrf_security`
* :ref:`Wi-Fi credentials <zephyr:lib_wifi_credentials>`
