.. _nrf_cloud_mqtt_device_message:

Cellular: nRF Cloud MQTT device message
#######################################

.. contents::
   :local:
   :depth: 2

The nRF Cloud MQTT device message sample demonstrates how to use the `nRF Cloud MQTT API`_ to send `device messages <nRF Cloud Device Messages_>`_.
Every button press sends a message to nRF Cloud.

It also demonstrates the use of the :ref:`lib_nrf_cloud_alert` and :ref:`lib_nrf_cloud_log` libraries.
The sample sends an alert when the device first comes online.
It also sends a log message indicating the sample version when the button is pressed.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be onboarded with `nRF Cloud`_.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_mqtt_device_message_onboarding_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 DK, or modem firmware v2.x.x for the nRF9161 and nRF9151 DKs.

User interface
**************

Button 1:
   Press to send a device message to nRF Cloud.

LED 1:
   Indicates that the device is connected to LTE and is ready to send messages.

LED 2:
   Indicates that a message is being sent to nRF Cloud.

.. _nrf_cloud_mqtt_device_message_onboarding:

Configuration
*************

|config|

Setup
=====

You must onboard your device to nRF Cloud for this sample to function.
You only need to do this once for each device.

To onboard your device, install `nRF Cloud Utils`_ and follow the instructions in the README.

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_mqtt_device_message`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      *** Booting My Application v1.0.0-2d648132f3d7 ***
      *** Using nRF Connect SDK v3.1.99-2d648132f3d7 ***
      *** Using Zephyr OS v4.2.99-36d18bbab237 ***
      [00:00:00.288,421] <inf> nrf_cloud_mqtt_device_message: nRF Cloud MQTT Device Message Sample, version: 1.0.0
      [00:00:00.288,452] <inf> nrf_cloud_mqtt_device_message: Reset reason: 0x1
      [00:00:00.857,360] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
      [00:00:00.857,421] <inf> nrf_cloud_credentials: CA Size: 1824, AWS: Likely, CoAP: Likely
      [00:00:00.857,452] <inf> nrf_cloud_mqtt_device_message: nRF Cloud credentials detected!
      [00:00:00.864,044] <inf> nrf_cloud_mqtt_device_message: Enabling connectivity...
      +CGEV: EXCE STATUS 0
      +CEREG: 2,"81A6","03229B10",7
      %MDMEV: PRACH CE-LEVEL 0
      +CSCON: 1
      +CGEV: ME PDN ACT 0
      %MDMEV: SEARCH STATUS 2
      +CEREG: 1,"81A6","03229B10",7,,,"11100000","11100000"
      [00:00:02.621,734] <inf> nrf_cloud_mqtt_device_message: Connected to LTE
      [00:00:02.621,856] <inf> nrf_cloud_mqtt_device_message: Waiting for modem to acquire network time...
      %XTIME: "80","52010221044480","01"
      +CGEV: IPV6 0
      [00:00:05.622,772] <inf> nrf_cloud_mqtt_device_message: Network time obtained
      [00:00:05.625,183] <inf> nrf_cloud_info: Device ID: 12345678-1234-5678-9abc-def012345678
      [00:00:05.625,762] <inf> nrf_cloud_info: IMEI:      358240123456789
      [00:00:05.744,354] <inf> nrf_cloud_info: UUID:      12345678-1234-5678-9abc-def012345678
      [00:00:05.744,842] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.2
      [00:00:05.744,873] <inf> nrf_cloud_info: Protocol:          MQTT
      [00:00:05.744,903] <inf> nrf_cloud_info: Download protocol: HTTPS
      [00:00:05.744,903] <inf> nrf_cloud_info: Sec tag:           16842753
      [00:00:05.744,934] <inf> nrf_cloud_info: Host name:         mqtt.nrfcloud.com
      [00:00:05.744,964] <inf> nrf_cloud_mqtt_device_message: Connecting to nRF Cloud...
      [00:00:07.899,841] <inf> nrf_cloud_log: Changing cloud logging enabled to:1
      [00:00:08.379,699] <inf> nrf_cloud_mqtt_device_message: Connection to nRF Cloud ready
      [00:00:08.379,791] <inf> nrf_cloud_mqtt_device_message: Reset reason: 0x1
      [00:00:08.380,981] <inf> nrf_cloud_info: Team ID:   12345678-1234-5678-9abc-def012345670
      [00:00:08.487,213] <inf> nrf_cloud_mqtt_device_message: Sent Hello World message with ID: 1760964049761

Troubleshooting
===============

If you are not getting the output similar to the one in `Testing`_, check the following potential issue:

The network carrier does not provide date and time
   The sample requires the network carrier to provide date and time to the modem.
   Without a valid date and time, the modem cannot generate JWTs with an expiration time.

nRF Cloud logging
=================

To enable `Zephyr Logging`_ in nRF Cloud using the :ref:`lib_nrf_cloud_log` library, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=nrfcloud_logging.conf``

This KConfig fragment allows the sample and various subsystems that have logging enabled to send their logs to nRF Cloud.
Set the :kconfig:option:`CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL` Kconfig option to the log level of messages to be sent to nRF Cloud, such as ``4`` for debug log messages.

Querying device messages over REST API
**************************************

To query the device messages received by the `nRF Cloud`_ backend, send a GET request to the `ListMessages <nRF Cloud REST API ListMessages_>`_ endpoint.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_at_host`
* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_alert`
* :ref:`lib_nrf_cloud_log`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
