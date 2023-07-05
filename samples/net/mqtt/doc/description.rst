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
Subsequently, the sample connects to a configured MQTT server (default is `test.mosquitto.org`_), where it publishes messages to the topic ``<clientID>/my/publish/topic``.
You can also trigger message publication by pressing any of the buttons on the board.

The sample also subscribes to the topic ``<clientID>/my/subscribe/topic``, and receives any message published to that topic.

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
	Default is ``<clientID>/my/publish/topic``.

.. _CONFIG_MQTT_SAMPLE_TRANSPORT_SUBSCRIBE_TOPIC:

CONFIG_MQTT_SAMPLE_TRANSPORT_SUBSCRIBE_TOPIC - MQTT subscribe topic
	This configuration option sets the topic to which the sample subscribes.
	Default is ``<clientID>/my/subscribe/topic``.

.. include:: /includes/wifi_credentials_options.txt

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

For Thingy:91, with TLS and debug logging enabled for the :ref:`lib_mqtt_helper` library (for more information, see the related :ref:`sample output <mqtt_sample_output_IPv6>`):

.. code-block:: console

   west build -b thingy91_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-tls-nrf9160.conf -DCONFIG_MQTT_HELPER_LOG_LEVEL_DBG=y

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

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

The following serial UART output is displayed in the terminal emulator using a Wi-Fi connection, with the TLS overlay:

.. code-block:: console

      *** Booting Zephyr OS build v2.4.0-ncs1-rc1-6-g45f2d5cf8ea4  ***
      [00:00:05.996,520] <inf> network: Connecting to SSID: NORDIC-TEST
      [00:00:12.477,783] <inf> network: Wi-Fi Connected
      [00:00:17.997,253] <inf> transport: Connected to MQTT broker
      [00:00:18.007,049] <inf> transport: Hostname: test.mosquitto.org
      [00:00:18.009,981] <inf> transport: Client ID: F4CE37111350
      [00:00:18.013,519] <inf> transport: Port: 8883
      [00:00:18.018,341] <inf> transport: TLS: Yes
      [00:00:18.078,521] <inf> transport: Subscribed to topic F4CE37111350/my/subscribe/topic
      [00:01:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 61458" on topic: "F4CE37111350/my/publish/topic"
      [00:02:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 121458" on topic: "F4CE37111350/my/publish/topic"
      [00:03:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 181458" on topic: "F4CE37111350/my/publish/topic"
      [00:04:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 241458" on topic: "F4CE37111350/my/publish/topic"
      [00:05:01.475,982] <inf> transport: Publishing message: "Hello MQTT! Current uptime is: 301459" on topic: "F4CE37111350/my/publish/topic"

.. _mqtt_sample_output_IPv6:

The sample output showing IPv6, but for a different build configuration using LTE on the Thingy:91 with the TLS overlay, and debug logging enabled for the :ref:`lib_mqtt_helper` library:

.. code-block:: console

      *** Booting Zephyr OS build v3.2.99-ncs2-34-gf8f113382356 ***
      [00:00:00.286,254] <inf> network: Bringing network interface up
      [00:00:00.286,621] <dbg> mqtt_helper: mqtt_state_set: State transition: MQTT_STATE_UNINIT --> MQTT_STATE_DISCONNECTED
      [00:00:00.310,028] <dbg> mqtt_helper: mqtt_helper_poll_loop: Waiting for connection_poll_sem
      [00:00:01.979,553] <inf> network: Connecting...
      [00:00:04.224,426] <inf> network: IP Up
      [00:00:09.233,612] <dbg> mqtt_helper: broker_init: Resolving IP address for test.mosquitto.org
      [00:00:10.541,839] <dbg> mqtt_helper: broker_init: IPv6 Address found 2001:41d0:1:925e::1 (AF_INET6)
      [00:00:10.541,900] <dbg> mqtt_helper: mqtt_state_set: State transition: MQTT_STATE_DISCONNECTED --> MQTT_STATE_TRANSPORT_CONNECTING
      [00:00:13.747,406] <dbg> mqtt_helper: mqtt_state_set: State transition: MQTT_STATE_TRANSPORT_CONNECTING --> MQTT_STATE_TRANSPORT_CONNECTED
      [00:00:13.747,467] <dbg> mqtt_helper: mqtt_state_set: State transition: MQTT_STATE_TRANSPORT_CONNECTED --> MQTT_STATE_CONNECTING
      [00:00:13.747,497] <dbg> mqtt_helper: client_connect: Using send socket timeout of 60 seconds
      [00:00:13.747,497] <dbg> mqtt_helper: mqtt_helper_connect: MQTT connection request sent
      [00:00:13.747,558] <dbg> mqtt_helper: mqtt_helper_poll_loop: Took connection_poll_sem
      [00:00:13.747,558] <dbg> mqtt_helper: mqtt_helper_poll_loop: Starting to poll on socket, fd: 0
      [00:00:13.747,589] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:00:14.370,727] <dbg> mqtt_helper: mqtt_evt_handler: MQTT mqtt_client connected
      [00:00:14.370,788] <dbg> mqtt_helper: mqtt_state_set: State transition: MQTT_STATE_CONNECTING --> MQTT_STATE_CONNECTED
      [00:00:14.370,788] <inf> transport: Connected to MQTT broker
      [00:00:14.370,819] <inf> transport: Hostname: test.mosquitto.org
      [00:00:14.370,849] <inf> transport: Client ID: 350457791735879
      [00:00:14.370,880] <inf> transport: Port: 8883
      [00:00:14.370,880] <inf> transport: TLS: Yes
      [00:00:14.370,910] <dbg> mqtt_helper: mqtt_helper_subscribe: Subscribing to: F4CE37111350/my/subscribe/topic
      [00:00:14.494,354] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:00:15.136,047] <dbg> mqtt_helper: mqtt_evt_handler: MQTT_EVT_SUBACK: id = 2469 result = 0
      [00:00:15.136,077] <inf> transport: Subscribed to topic F4CE37111350/my/subscribe/topic
      [00:00:15.136,108] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:00:15.136,260] <dbg> mqtt_helper: mqtt_evt_handler: MQTT_EVT_PUBLISH, message ID: 52428, len = 850
      [00:00:15.136,444] <inf> transport: Received payload: $ on topic: F4CE37111350/my/subscribe/topic
      [00:00:15.136,444] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:00:44.495,147] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:00:45.478,210] <dbg> mqtt_helper: mqtt_evt_handler: MQTT_EVT_PINGRESP
      [00:00:45.478,210] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:01:00.492,370] <dbg> mqtt_helper: mqtt_helper_publish: Publishing to topic: F4CE37111350/my/publish/topic
      [00:01:00.493,133] <inf> transport: Published message: "Hello MQTT! Current uptime is: 60492" on topic: "F4CE37111350/my/publish/topic"
      [00:01:01.270,690] <dbg> mqtt_helper: mqtt_evt_handler: MQTT_EVT_PUBACK: id = 60492 result = 0
      [00:01:01.270,690] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:01:05.093,719] <dbg> mqtt_helper: mqtt_evt_handler: MQTT_EVT_PUBLISH, message ID: 52428, len = 32
      [00:01:05.093,872] <inf> transport: Received payload: Test message from mosquitto_pub! on topic: F4CE37111350/my/subscribe/topic
      [00:01:05.093,872] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:01:30.503,021] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:01:31.494,537] <dbg> mqtt_helper: mqtt_evt_handler: MQTT_EVT_PINGRESP
      [00:01:31.494,567] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:02:00.492,462] <dbg> mqtt_helper: mqtt_helper_publish: Publishing to topic: F4CE37111350/my/publish/topic
      [00:02:00.501,678] <inf> transport: Published message: "Hello MQTT! Current uptime is: 120492" on topic: "F4CE37111350/my/publish/topic"
      [00:02:00.503,692] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0
      [00:02:01.577,453] <dbg> mqtt_helper: mqtt_evt_handler: MQTT_EVT_PUBACK: id = 54956 result = 0
      [00:02:01.577,484] <dbg> mqtt_helper: mqtt_helper_poll_loop: Polling on socket fd: 0

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
* :ref:`net_if_interface`

It uses the following libraries for nRF7002 DK builds:

* :ref:`nrf_security`
* :ref:`net_if_interface`
