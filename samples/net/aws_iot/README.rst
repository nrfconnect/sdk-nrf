.. _aws_iot:

AWS IoT
#######

.. contents::
   :local:
   :depth: 2

The Amazon Web Services Internet-of-Things (AWS IoT) sample demonstrates how to connect an nRF91 Series or nRF70 Series device to the `AWS IoT Core`_ service over MQTT to publish and receive messages.
This sample showcases the use of the :ref:`lib_aws_iot` library, which includes support for FOTA using the :ref:`lib_aws_fota` library.

.. |wifi| replace:: Wi-Fi®

.. include:: /includes/net_connection_manager.txt

Before this sample can be used, an AWS IoT server instance needs to be setup in order for the device to connect to it.
Refer to :ref:`aws_iot_sample_server_setup` to complete the necessary steps.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, the sample supports emulation using :ref:`QEMU x86 <zephyr:qemu_x86>`.

.. include:: /includes/tfm.txt

Overview
********

The sample publishes messages in the `JSON`_ string format to the `AWS IoT Device Shadow Service`_, addressed to the `AWS IoT Device Shadow Topic <AWS IoT Device Shadow Topics_>`_: ``$aws/things/<thingname>/shadow/update``.
Messages are published once at an interval set by the :ref:`CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS` Kconfig option.
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

Configuration
*************

|config|

.. _aws_iot_sample_server_setup:

Setup
=====

To run this sample and connect to AWS IoT, complete the steps described in the :ref:`aws_setup_and_configuration` section of the AWS IoT library documentation.
This is to obtain the AWS IoT broker *hostname* and the *client ID* of the device and provision a device certificate to a *security tag*.

The corresponding options that must be set for each of these values are:

* :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG`
* :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

Set these options in the project configuration file located at :file:`samples/net/aws_iot/prj.conf`.
For documentation related to FOTA DFU, see :ref:`lib_aws_fota`.

.. note::
   For nRF70 Series devices, certificates must be provisioned at runtime.
   This is achieved by pasting the PEM content into the respective files in the :file:`certs/` subdirectory and ensuring the :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option is enabled.

Configuration options
=====================

Check and configure the following Kconfig options:

General options
---------------

The following lists the application-specific configurations used in the sample.
They are located in :file:`samples/net/aws_iot/Kconfig`.

.. _CONFIG_AWS_IOT_SAMPLE_APP_VERSION:

CONFIG_AWS_IOT_SAMPLE_APP_VERSION
   This configuration option sets the application version number.

.. _CONFIG_AWS_IOT_SAMPLE_JSON_MESSAGE_SIZE_MAX:

CONFIG_AWS_IOT_SAMPLE_JSON_MESSAGE_SIZE_MAX
   This configuration option sets the maximum size of JSON messages that are sent to AWS IoT.

.. _CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS:

CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS
   This configuration option configures the time interval between each message publication.

.. _CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS:

CONFIG_AWS_IOT_SAMPLE_CONNECTION_RETRY_TIMEOUT_SECONDS
   This configuration option configures the number of seconds between each AWS IoT connection retry.
   If the connection is lost, the sample attempts to reconnect at the frequency set by this option.

.. _CONFIG_AWS_IOT_SAMPLE_DEVICE_ID_USE_HW_ID:

CONFIG_AWS_IOT_SAMPLE_DEVICE_ID_USE_HW_ID
   This configuration option configures the sample to use the HW ID (IMEI, MAC) as the device ID in the MQTT connection.

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

Configuration files
===================

The sample includes pre-configured configuration files for the development kits that are supported:

* :file:`prj.conf` - General configuration file for all devices.
* :file:`boards/nrf9151dk_nrf9151_ns.conf` - Configuration file for the nRF9151 DK.
* :file:`boards/nrf9161dk_nrf9161_ns.conf` - Configuration file for the nRF9161 DK.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file for the Thingy:91.
* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.conf` - Configuration file for the nRF7002 DK.
* :file:`boards/qemu_x86.conf` - Configuration file for QEMU x86.

The following configuration and DTS overlay files are included to host the MCUboot secondary image slot on external flash for the nRF7002 DK:

* :file:`boards/nrf7002dk_nrf5340_cpuapp_ns.overlay` - DTS overlay file for the application image.
* :file:`child_image/mcuboot/nrf7002dk_nrf5340_cpuapp.overlay` - DTS overlay file for the MCUboot child image.
* :file:`child_image/mcuboot/nrf7002dk_nrf5340_cpuapp.conf` - Configuration file for the MCUboot child image.

Files that are located under the :file:`/boards` folder are automatically merged with the :file:`prj.conf` file when you build for the corresponding target.
Files that are located under the :file:`/child_image/mcuboot` folder are used to configure the MCUboot child image.
To read more about child images, see :ref:`ug_multi_image`.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

.. _aws_iot_sample_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/net/aws_iot`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Reset your board.
#. Observe that the board connects to the network and the AWS IoT broker.
   After the connection has been established, the device starts to publish messages to the AWS IoT shadow service.
   The frequency of the messages that are published to the broker can be set by :ref:`CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS <CONFIG_AWS_IOT_SAMPLE_PUBLICATION_INTERVAL_SECONDS>`.

Sample output
=============

The following serial UART output is displayed in the terminal emulator when running the sample.
The *modem_version* parameter in messages published to AWS IoT will not be present for Wi-Fi builds.

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
   This library makes it possible to send AT commands to the nRF91 Series modem and receive responses using the `Cellular Monitor`_ app from nRF Connect for Desktop.
   The additional logs are AT command responses that the modem sends to the application core that are forwarded over UART to be displayed on any of these nRF Connect for Desktop apps.

To observe incoming messages in the AWS IoT console, follow the steps documented in :ref:`aws_iot_testing_and_debugging`.

Emulation
=========

The sample can run on :ref:`QEMU x86 <zephyr:qemu_x86>`, which simplifies development and testing and removes the need for hardware.
Before you build and run on QEMU x86, you need to perform the steps documented in :ref:`networking_with_qemu`.

When these steps are completed, you can build and run the sample by using the following commands:

.. code-block:: console

   west build -b qemu_x86 samples/net/mqtt
   west build -t run

Troubleshooting
===============

To enable more verbose logging from the AWS IoT library, enable the :kconfig:option:`CONFIG_AWS_IOT_LOG_LEVEL_DBG` option.

* If you have issues with connectivity on nRF91 Series devices, see the `Cellular Monitor`_ documentation to learn how to capture modem traces in order to debug network traffic in Wireshark.
  The sample enables modem traces by default.
* If you have issues with the sample, refer to :ref:`testing`.
* For issues related to connection towards AWS IoT, refer to :ref:`AWS IoT library troubleshooting <aws_iot_troubleshooting>`.

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`lib_aws_iot`
* :ref:`net_mgmt_interface`
* :ref:`lib_hw_id`
