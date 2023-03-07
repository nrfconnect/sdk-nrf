.. _mqtt_sample_description:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The MQTT sample communicates with an MQTT broker either over LTE using the nRF9160 DK or Thingy:91, or over Wi-Fi using the nRF7002 DK.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, the sample supports emulation using :ref:`Native Posix <zephyr:native_posix>`.

Overview
*********

The sample connects to either LTE or Wi-Fi, depending on the board for which the sample is compiled.
Subsequently, the sample connects to a configured MQTT server (default is `test.mosquitto.org`_), where it publishes messages to the topic ``my/publish/topic``.
You can also trigger message publication by pressing any of the buttons on the board.

The sample also subscribes to the topic ``my/subscribe/topic``, and receives any message published to that topic.
The sample supports Transport Layer Security (TLS) and it can be enabled through overlay configuration files included in the sample.

.. note::
   When enabling TLS and building for nRF9160 based boards, the size of the incoming message cannot exceed 2 kB.
   This is due to a limitation in the modem's internal TLS buffers.

Configuration
*************

|config|


Configuration options
=====================

Check and configure the following Kconfig options:

General options
---------------

.. _CONFIG_MQTT_SAMPLE_TRIGGER_TIMEOUT_SECONDS:

CONFIG_MQTT_SAMPLE_TRIGGER_TIMEOUT_SECONDS - Trigger timeout
	This configuration option sets the interval at which the sample publishes a message to the MQTT broker.

.. _CONFIG_MQTT_SAMPLE_TRANSPORT_RECONNECTION_TIMEOUT_SECONDS:

CONFIG_MQTT_SAMPLE_TRANSPORT_RECONNECTION_TIMEOUT_SECONDS - Transport reconnection timeout
	This configuration option sets the interval at which the sample tries to reconnect to the MQTT broker upon a lost connection.

.. _CONFIG_MQTT_SAMPLE_TRANSPORT_BROKER_HOSTNAME:

CONFIG_MQTT_SAMPLE_TRANSPORT_BROKER_HOSTNAME - MQTT broker hostname
	This configuration sets the MQTT broker hostname.
	Default is `test.mosquitto.org`_.

.. _CONFIG_MQTT_SAMPLE_TRANSPORT_CLIENT_ID:

CONFIG_MQTT_SAMPLE_TRANSPORT_CLIENT_ID - MQTT client ID
	This configuration sets the MQTT client ID name.
	If not set, the client ID will default to the modem's IMEI number for nRF9160 boards, MAC address for the nRF7002 DK, or a random number for Native Posix.

.. _CONFIG_MQTT_SAMPLE_TRANSPORT_PUBLISH_TOPIC:

CONFIG_MQTT_SAMPLE_TRANSPORT_PUBLISH_TOPIC - MQTT publish topic
	This configuration option sets the topic to which the sample publishes messages.
	Default is ``my/publish/topic``.

.. _CONFIG_MQTT_SAMPLE_TRANSPORT_SUBSCRIBE_TOPIC:

CONFIG_MQTT_SAMPLE_TRANSPORT_SUBSCRIBE_TOPIC - MQTT subscribe topic
	This configuration option sets the topic to which the sample subscribes.
	Default is ``my/subscribe/topic``.

Wi-Fi options
-------------

* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC` - This option enables static Wi-Fi configuration.
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_SSID` - Wi-Fi SSID.
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD` - Wi-Fi password.
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_OPEN` - Wi-Fi network uses no password.
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_PSK` - Wi-Fi network uses a password and PSK security (default).
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_PSK_SHA256` - Wi-Fi network uses a password and PSK-256 security.
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_SAE` - Wi-Fi network uses a password and SAE security.

Additional configuration
========================

Check and configure the following library Kconfig options specific to the MQTT helper library:

 * :kconfig:option:`CONFIG_MQTT_HELPER_PORT` - This option sets the MQTT broker port.
 * :kconfig:option:`CONFIG_MQTT_HELPER_SEC_TAG` - This option sets the MQTT connection with TLS security tag.

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* :file:`prj.conf` - General project configuration file.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file for the Thingy:91.
* :file:`boards/nrf7002dk_nrf5340_cpuapp.conf` - Configuration file for the nRF7002 DK.
* :file:`boards/native_posix.conf` - Configuration file for native posix.

Files that are located under the :file:`/boards` folder is automatically merged with the :file:`prj.conf` file when you build for corresponding target.

In addition, the sample provides the following overlay configuration files, which are used to enable additional features in the sample:

* :file:`overlay-tls-nrf9160.conf` - TLS overlay configuration file for nRF9160 DK and Thingy:91.
* :file:`overlay-tls-nrf7002.conf` - TLS overlay configuration file for nRF7002 DK.
* :file:`overlay-tls-native_posix.conf` - TLS overlay configuration file for native posix.

They are located in :file:`samples/net/mqtt` folder.

To add a specific overlay configuration file to the build, add the ``-- -DOVERLAY_CONFIG=<overlay_config_file>`` flag to your build.

See :ref:`cmake_options` for instructions on how to add this option to your build.
For example, when building with the command line, the following commands can be used for the nRF9160 DK:

  .. code-block:: console

     west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-tls-nrf9160.conf

Building and running
********************

.. |sample path| replace:: :file:`samples/net/mqtt`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Reset your board.
#. Observe that the board connects to the network and the configured MQTT broker (:ref:`CONFIG_MQTT_SAMPLE_TRANSPORT_BROKER_HOSTNAME <CONFIG_MQTT_SAMPLE_TRANSPORT_BROKER_HOSTNAME>`).
   When a network connection has been established, LED 1 (green) on the board lights up.
   After the connection has been established the board starts to publish messages to the topic set by :ref:`CONFIG_MQTT_SAMPLE_TRANSPORT_PUBLISH_TOPIC <CONFIG_MQTT_SAMPLE_TRANSPORT_PUBLISH_TOPIC>`.
   The frequency of the messages that are published to the broker can be set by :ref:`CONFIG_MQTT_SAMPLE_TRIGGER_TIMEOUT_SECONDS <CONFIG_MQTT_SAMPLE_TRIGGER_TIMEOUT_SECONDS>` or triggered asynchronously by pressing any of the buttons on the board.
   At any time, the sample can receive messages published to the subscribe topic set by :ref:`CONFIG_MQTT_SAMPLE_TRANSPORT_SUBSCRIBE_TOPIC <CONFIG_MQTT_SAMPLE_TRANSPORT_SUBSCRIBE_TOPIC>`.
#. Use an MQTT client like `Mosquitto`_ or `VSMQTT`_ to subscribe to, and publish data to the broker.

Sample output
=============

The following serial UART output is displayed in the terminal emulator:

.. code-block:: console

      *** Booting Zephyr OS build v2.4.0-ncs1-rc1-6-g45f2d5cf8ea4  ***
      [00:00:05.996,520] <inf> network: Connecting to SSID: NORDIC-TEST
      [00:00:12.477,783] <inf> network: Wi-Fi Connected
      [00:00:17.997,253] <inf> transport: Connected to MQTT broker
      [00:00:18.007,049] <inf> transport: Hostname: test.mosquitto.org
      [00:00:18.009,981] <inf> transport: Client ID: F4CE37111350
      [00:00:18.013,519] <inf> transport: Port: 8883
      [00:00:18.018,341] <inf> transport: TLS: Yes
      [00:00:18.078,521] <inf> transport: Subscribed to topic my/subscribe/topic
      [00:01:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 61458" on topic: "my/publish/topic"
      [00:02:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 121458" on topic: "my/publish/topic"
      [00:03:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 181458" on topic: "my/publish/topic"
      [00:04:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 241458" on topic: "my/publish/topic"
      [00:05:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 301459" on topic: "my/publish/topic"

Reconnection logic
------------------

If the network connection is lost, the following log output is displayed:

  .. code-block:: console

     <inf> network: Disconnected

If this occurs, the network stack automatically reconnects to the network using its built-in backoff functionality.

If the TCP/IP (MQTT) connection is lost, the following log output is displayed:

  .. code-block:: console

     <inf> transport: Disconnected from MQTT broker

If this occurs, the sample's transport module has built-in reconnection logic that will try to reconnect at the frequency set by
:ref:`CONFIG_MQTT_SAMPLE_TRANSPORT_RECONNECTION_TIMEOUT_SECONDS <CONFIG_MQTT_SAMPLE_TRANSPORT_RECONNECTION_TIMEOUT_SECONDS>`.

Emulation
=========

The sample can be run in :ref:`Native Posix <zephyr:native_posix>` that simplifies development and testing and removes the need for hardware.
Before you can build and run native posix, you need to perform the steps included in this link: :ref:`networking_with_native_posix`.

When the aforementioned steps are completed, you can build and run the sample by using the following commands:

.. code-block:: console

   west build -b native_posix samples/net/mqtt
   west build -t run

Troubleshooting
===============

* If you are having issues with connectivity on nRF9160 based boards, see the `Trace Collector`_ documentation to learn how to capture modem traces in order to debug network traffic in Wireshark.
* Public MQTT brokers might be unstable.
  If you have trouble connecting to the MQTT broker, try switching to another broker by changing the value of the :ref:`CONFIG_MQTT_SAMPLE_TRANSPORT_BROKER_HOSTNAME <CONFIG_MQTT_SAMPLE_TRANSPORT_BROKER_HOSTNAME>` configuration option.
  If you are switching to another broker, remember to update the CA certificate. To know more on certificates and provisioning, see :ref:`mqtt_sample_provisioning`.

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`mqtt_socket_interface`
* :ref:`zbus`
* :ref:`smf`

It uses the following libraries and secure firmware component for nRF9160 DK and Thingy:91 builds:

* :ref:`lte_lc_readme`
* :ref:`nrfxlib:nrf_modem`
* :ref:`Trusted Firmware-M <ug_tfm>`

It uses the following libraries for nRF7002 DK builds:

* :ref:`nrfxlib:nrf_security`
* :ref:`net_if_interface`
