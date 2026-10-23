.. _aws_iot:

.. ncs-sample::
   :title: AWS IoT

   The AWS IoT sample demonstrates how to connect to the `AWS IoT Core`_ service over MQTT to publish and receive messages.
   The sample connects either to an LTE network using an nRF91 Series device, or to Wi-Fi® using an nRF71 Series or nRF70 Series device combined with a compatible host platform.
   It uses the :ref:`lib_aws_iot` library, which includes support for firmware over-the-air (FOTA) update using the :ref:`lib_aws_fota` library.

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

The sample publishes messages in the `JSON`_ string format to the `AWS IoT Device Shadow Service`_, addressed to the `AWS IoT Device Shadow Topic <AWS IoT Device Shadow Topics_>`_: ``$aws/things/<thingname>/shadow/update``.
Messages are published once at an interval set by the :option:`CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS` Kconfig option.
The message has the following structure:

.. parsed-literal::
   :class: highlight

	{
		"state": {
			"reported": {
				"app_version": "v1.0.0",
				"modem_version": "nrf9160-1.3.4", /\* Not present for Wi-Fi builds \*/
				"uptime": 2469
			}
		}
	}

The message comprises of three parameters the application version, the modem version string, and an uptime timestamp.
The firmware version strings can be used to verify that the device has updated the corresponding image post FOTA DFU.
See the :ref:`lib_aws_fota` library for information on how to create FOTA jobs.

In addition to publishing data to the AWS IoT shadow, the sample also subscribes to the following topics:

+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
|            Topic                                 | Kconfig option                                                 | Purpose                                                                                                             |
+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| ``$aws/things/<thing-name>/shadow/update/delta`` | :kconfig:option:`CONFIG_AWS_IOT_TOPIC_UPDATE_DELTA_SUBSCRIBE`  | Topic that receives the difference between the reported and desired shadow states.                                  |
|                                                  |                                                                | Can be used for run-time configuration of the device.                                                               |
+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| ``$aws/things/<thing-name>/shadow/get/accepted`` | :kconfig:option:`CONFIG_AWS_IOT_TOPIC_GET_ACCEPTED_SUBSCRIBE`  | Topic that receives the shadow document whenever it is requested                                                    |
|                                                  |                                                                | by the device. Auto-requesting of the shadow document after an established connection                               |
|                                                  |                                                                | can be enabled by setting the :kconfig:option:`CONFIG_AWS_IOT_AUTO_DEVICE_SHADOW_REQUEST` option.                   |
+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| ``$aws/things/<thing-name>/shadow/get/rejected`` | :kconfig:option:`CONFIG_AWS_IOT_TOPIC_GET_REJECTED_SUBSCRIBE`  | Topic that receives an error status message if requesting the device shadow failed.                                 |
+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| ``my-custom-topic/example``                      | NA - Non-shadow topic                                          | Dummy application-specific topic. Can be used for anything.                                                         |
+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+
| ``my-custom-topic/example_2``                    | NA - Non-shadow topic                                          | Dummy application-specific topic. Can be used for anything.                                                         |
+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+

The application-specific topics are not part of the AWS IoT shadow service and must therefore be passed to the :ref:`lib_aws_iot` library using the :c:func:`aws_iot_application_topics_set` function before connecting.
How to add application-specific topics is documented in the :ref:`AWS IoT library usage <aws_iot_usage>` section.

FOTA
====

The sample supports FOTA updates using the :ref:`lib_aws_fota` library.
The sample supports FOTA on nRF91 Series devices, the nRF7120 DK, and the nRF7002 DK.
The sample does not support FOTA on other nRF71 Series and nRF70 Series configurations.
The board configuration files enable it.

To download the firmware over HTTPS, you must set the :kconfig:option:`CONFIG_AWS_FOTA_DOWNLOAD_SECURITY_TAG` Kconfig option to a security tag that contains the CA certificate of the firmware host.
In this sample, the nRF7120 DK and nRF7002 DK board configuration files set the option to ``201``, the same value as :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`, because the CA certificate provisioned for the MQTT connection also validates Amazon S3.
If you change :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`, you must also change :kconfig:option:`CONFIG_AWS_FOTA_DOWNLOAD_SECURITY_TAG` or provision the CA certificate under the FOTA security tag.
For more information, see the :ref:`Requirements <aws_fota_requirements>` section of the :ref:`lib_aws_fota` library.

On the nRF7002 DK, the primary and secondary MCUboot slots do not both fit into the internal flash.
Therefore, the :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.overlay` file moves ``slot1_partition`` to the external flash.

.. _aws_iot_sample_server_setup:

Setting up AWS IoT
==================

Before you can use this sample, you must set up an AWS IoT server instance for the device to connect to.
Complete the steps described in the :ref:`aws_setup_and_configuration` section of the AWS IoT library documentation.
This gives you the AWS IoT broker *hostname* and the *client ID* of the device, and provisions a device certificate to a *security tag*.

Set the following options in the project configuration file :file:`samples/net/aws_iot/prj.conf`:

* :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`
* :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

For information about FOTA, see the `FOTA`_ section.

.. note::
   On nRF91 Series devices, you can provision certificates only while the modem is not connected to the network.
   On nRF71 Series and nRF70 Series devices, certificates are provisioned at runtime.
   To provision them, paste the PEM content into the files in the :file:`certs/` subdirectory and enable the :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/net/aws_iot/Kconfig`):

.. options-from-kconfig::
   :show-type:

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* :file:`prj.conf` - General configuration file for all devices.
* :file:`boards/nrf9151dk_nrf9151_ns.conf` - Configuration file for the nRF9151 DK.
* :file:`boards/nrf9161dk_nrf9161_ns.conf` - Configuration file for the nRF9161 DK.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file for the Thingy:91.
* :file:`boards/thingy91x_nrf9151_ns.conf` - Configuration file for the Thingy:91 X.
* :file:`boards/native_sim.conf` - Configuration file for the native simulator emulation.
* :file:`boards/nrf7120dk_nrf7120_cpuapp_ns.conf` - Configuration file for the nRF7120 DK.
* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - Configuration file for the nRF7002 DK.
* :file:`boards/nrf54lm20dk_nrf54lm20a_cpuapp.conf` - Configuration file for the nRF54LM20 DK (nRF54LM20A SoC) with the nRF7002-EB II.
* :file:`boards/nrf54lm20dk_nrf54lm20b_cpuapp.conf` - Configuration file for the nRF54LM20 DK (nRF54LM20B SoC) with the nRF7002-EB II.
* :file:`wifi.conf` - Wi-Fi networking configuration, common to the nRF71 Series and nRF70 Series devices.
  Must be added explicitly for Wi-Fi builds.

The following DTS overlay and configuration files are included to host the MCUboot secondary image slot on external flash for the nRF7002 DK:

* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.overlay` - DTS overlay file for the application image.
* :file:`sysbuild/mcuboot/nrf7002dk_nrf5340_cpuapp.overlay` - DTS overlay file for the MCUboot image.
* :file:`sysbuild/mcuboot/nrf7002dk_nrf5340_cpuapp.conf` - Configuration file for the MCUboot image.

The following DTS overlay file is included to increase the non-secure RAM partition size to provide more memory for the application:

* :file:`boards/nrf7120dk_nrf7120_cpuapp_ns.overlay` - DTS overlay file for the nRF7120 DK.

Board files under the :file:`boards/` folder are merged automatically for the selected target.
Files under the :file:`sysbuild/mcuboot/` folder configure the MCUboot image.
The :file:`wifi.conf` file is not board-specific and must be passed with ``aws_iot_EXTRA_CONF_FILE``.

To add a specific extra configuration file to the build, add the ``-- -Daws_iot_EXTRA_CONF_FILE=<extra_conf_file>`` flag to your west build command.

Wi-Fi
-----

The default configuration does not include networking.
Add it for the nRF71 Series and nRF70 Series devices with the :file:`wifi.conf` file:

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp/ns -- -Daws_iot_EXTRA_CONF_FILE="wifi.conf"

To build for the nRF54LM20 DK with the nRF7002-EB II, also set the shield and the snippets:

.. code-block:: console

   west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp -- -Daws_iot_SHIELD="nrf7002eb2" -Daws_iot_SNIPPET="nrf70-wifi;wifi-credentials" -Daws_iot_EXTRA_CONF_FILE="wifi.conf"

.. _aws_iot_sample_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/net/aws_iot`

.. include:: /includes/build_and_run_ns.txt

To build and run the sample on the :zephyr:board:`native simulator <native_sim>`, first complete the steps in :ref:`networking_with_native_sim`.
Then use the following commands:

.. code-block:: console

   west build -b native_sim -d build samples/net/aws_iot
   ./build/aws_iot/zephyr/zephyr.exe

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset the kit.
#. Observe that the kit connects to the network and the AWS IoT broker.
   After the connection is established, the device starts to publish messages to the AWS IoT shadow service.
   The interval between messages is set by the :option:`CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS` Kconfig option.

Sample output
=============

The following output is displayed in the terminal emulator when running the sample.
The *modem_version* parameter in messages published to AWS IoT is not present for Wi-Fi builds.

.. code-block:: console

   *** Booting Zephyr OS build v3.3.99-ncs1-2858-gc9d01d05ce83 ***
   [00:00:00.252,838] <inf> aws_iot_sample: The AWS IoT sample started, version: v1.0.0
   [00:00:00.252,868] <inf> aws_iot_sample: Bringing network interface up and connecting to the network
   [00:00:02.486,297] <inf> aws_iot_sample: Network connectivity established
   [00:00:07.486,419] <inf> aws_iot_sample: Connecting to AWS IoT
   [00:00:11.061,981] <inf> aws_iot_sample: AWS_IOT_EVT_CONNECTED
   [00:00:11.062,866] <inf> aws_iot_sample: Publishing message: {"state":{"reported":{"uptime":11062,"app_version":"v1.0.0","modem_version":"nrf9160_1.3.4"}}} to AWS IoT shadow
   [00:01:11.073,120] <inf> aws_iot_sample: Publishing message: {"state":{"reported":{"uptime":71063,"app_version":"v1.0.0","modem_version":"nrf9160_1.3.4"}}} to AWS IoT shadow

.. note::
   For nRF91 Series devices, the output differs from the above example output.
   This is because the sample enables the :ref:`lib_at_host` library using the :kconfig:option:`CONFIG_AT_HOST_LIBRARY` option.
   This library makes it possible to send AT commands to the nRF91 Series modem and receive responses using the `Cellular Monitor app`_ in nRF Connect for Desktop.
   The additional logs are AT command responses that the modem sends to the application core that are forwarded over UART to be displayed on any of these nRF Connect for Desktop apps.

To observe incoming messages in the AWS IoT console, follow the steps documented in :ref:`aws_iot_testing_and_debugging`.

Troubleshooting
===============

If you have issues with connectivity on nRF91 Series devices, see the `Cellular Monitor app`_ documentation to learn how to capture modem traces to debug network traffic in Wireshark.
This sample enables modem traces by default.

If you have issues with connectivity on nRF71 Series or nRF70 Series devices, see the :ref:`wifi_monitor_sample` sample documentation to learn how to capture and analyze Wi-Fi traffic to debug connectivity issues.
Also verify that the Wi-Fi credentials configured for your device match your access point, as described in the `Configuration options`_ section on this page.

To enable more verbose logging from the AWS IoT library, enable the :kconfig:option:`CONFIG_AWS_IOT_LOG_LEVEL_DBG` option.

* If you have issues with the sample, refer to :ref:`testing`.
* For issues related to connection towards AWS IoT, refer to :ref:`AWS IoT library troubleshooting <aws_iot_troubleshooting>`.
* If the connection to the server times out with error -116 (``ETIMEDOUT``), increase the :kconfig:option:`CONFIG_NET_SOCKETS_CONNECT_TIMEOUT` value.
  The default timeout might be insufficient for Wi-Fi connections that require a full TLS handshake.

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`lib_aws_iot`
* :ref:`lib_hw_id`
* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
* :ref:`modem_key_mgmt`
* :ref:`lte_lc_readme`
* :ref:`nrfxlib:nrf_modem`
* :ref:`Trusted Firmware-M <ug_tfm>`
