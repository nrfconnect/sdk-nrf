.. _aws_iot:

AWS IoT
#######

.. contents::
   :local:
   :depth: 2

The Amazon Web Services Internet-of-Things (AWS IoT) sample demonstrates how to connect an nRF91 Series or nRF70 Series device to the `AWS IoT Core`_ service over MQTT to publish and receive messages.
This sample showcases the use of the :ref:`lib_aws_iot` library, which includes support for FOTA using the :ref:`lib_aws_fota` library.

Before this sample can be used, an AWS IoT server instance needs to be setup in order for the device to connect to it.
Refer to :ref:`aws_iot_sample_server_setup` to complete the nessecary steps.

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
| ``my-custom-topic/example``                      | NA - Non-shadow topic                                          | Dummy application-specific topic. Can be used for anything.                                                         |
+--------------------------------------------------+----------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------+

The application-specific topics are not part of the AWS IoT shadow service and must therefore be passed to the :ref:`lib_aws_iot` library using the :c:func:`aws_iot_subscription_topics_add` function before connecting.
How to add application-specific topics is documented in the :ref:`AWS IoT library usage <aws_iot_usage>` section.

Configuration
*************

|config|

.. _aws_iot_sample_server_setup:

Setup
=====

To run this sample and connect to AWS IoT, complete the steps described in the :ref:`lib_aws_iot` documentation.
This documentation retrieves the AWS IoT broker *hostname*, *security tag*, and *client ID*.
The corresponding options that must be set for each of these values are:

* :kconfig:option:`CONFIG_AWS_IOT_BROKER_HOST_NAME`
* :kconfig:option:`CONFIG_AWS_IOT_SEC_TAG`
* :kconfig:option:`CONFIG_AWS_IOT_CLIENT_ID_STATIC`

Set these options in the project configuration file located at :file:`samples/nrf9160/aws_iot/prj.conf`.
For documentation related to FOTA DFU, see :ref:`lib_aws_fota`.

Configuration options
=====================

Check and configure the following Kconfig options:

General options
---------------

The following lists the application-specific configurations used in the sample.
They are located in :file:`samples/nrf9160/aws_iot/Kconfig`.

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

.. include:: /includes/wifi_credentials_options.txt

Configuration files
===================

The sample includes preconfigured configuration files for the development kits that are supported:

* :file:`prj.conf` - General configuration file for all devices.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file for the Thingy:91.
* :file:`boards/nrf7002dk_nrf5340_cpuapp.conf` - Configuration file for the nRF7002 DK.
* :file:`boards/qemu_x86.conf` - Configuration file for QEMU x86.

The following configuration and DTS overlay files are included to host the MCUboot secondary image slot on external flash for the nRF7002 DK:

* :file:`boards/nrf7002dk_nrf5340_cpuapp.overlay` - DTS overlay file for the application image.
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

   *** Booting Zephyr OS build v3.3.99-ncs1-12-g2f5e820792d1 ***
   [00:00:00.253,204] <inf> aws_iot_sample: The AWS IoT sample started, version: v1.0.0
   [00:00:00.253,234] <inf> aws_iot_sample: Bringing network interface up and connecting to the network
   [00:00:03.736,450] <inf> aws_iot_sample: Network connectivity established
   [00:00:08.736,572] <inf> aws_iot_sample: Connecting to AWS IoT
   [00:00:08.736,633] <inf> aws_iot_sample: Next connection retry in 30 seconds
   [00:00:08.736,663] <inf> aws_iot_sample: AWS_IOT_EVT_CONNECTING
   [00:00:12.855,072] <inf> aws_iot_sample: AWS_IOT_EVT_CONNECTED
   [00:00:12.856,323] <inf> aws_iot_sample: Publishing message: {"state":{"reported":{"uptime":12855,"app_version":"v1.0.0","modem_version":"nrf9160_1.3.4"}}} to AWS IoT shadow
   [00:00:13.228,881] <inf> aws_iot_sample: AWS_IOT_EVT_READY
   [00:00:13.244,781] <inf> aws_iot_sample: AWS_IOT_EVT_PUBACK, message ID: 32367
   [00:01:12.867,675] <inf> aws_iot_sample: Publishing message: {"state":{"reported":{"uptime":72858,"app_version":"v1.0.0","modem_version":"nrf9160_1.3.4"}}} to AWS IoT shadow
   [00:02:12.883,789] <inf> aws_iot_sample: Publishing message: {"state":{"reported":{"uptime":132874,"app_version":"v1.0.0","modem_version":"nrf9160_1.3.4"}}} to AWS IoT shadow

.. note::
   For nRF91 Series devices, the output differs from the above example output.
   This is because the sample enables the :ref:`lib_at_host` library using the :kconfig:option:`CONFIG_AT_HOST_LIBRARY` option.
   This library makes it possible to send AT commands to the cellular modem and receive responses using the `LTE Link Monitor`_ or Cellular Monitor app from nRF Connect for Desktop.
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

* If you are having issues with connectivity on nRF91 Series devices, see the `Trace Collector`_ documentation to learn how to capture modem traces in order to debug network traffic in Wireshark.
  The sample enables modem traces by default, as set by the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` option.
* If you have issues with the sample, refer to :ref:`gs_testing`.
* For issues related to connection towards AWS IoT, refer to :ref:`AWS IoT library troubleshooting <aws_iot_troubleshooting>`.

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`lib_aws_iot`
* :ref:`net_mgmt_interface`
* :ref:`lib_hw_id`
