.. _nrf_mqtt_sn_publisher_sample:

.. ncs-sample::
   :title: MQTT-SN Publisher

   The MQTT-SN Publisher sample demonstrates how to connect to an MQTT-SN gateway and publish and subscribe to topics, using Zephyr's MQTT-SN client library.
   It runs on an nRF71 Series or nRF70 Series device connected over Wi-Fi®.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf7002dk_nrf5340_cpuapp, nrf7120dk_nrf7120_cpuapp

Overview
********

This is Zephyr's :zephyr:code-sample:`mqtt-sn-publisher` sample, built to run on the development kits listed above.

MQTT-SN is a lightweight publish/subscribe protocol derived from MQTT, adapted for constrained devices and non-TCP transports such as UDP.
The sample acts as an MQTT-SN v1.2 client: it connects to a gateway, which translates MQTT-SN messages to and from standard MQTT for a broker, publishes a periodic timestamp on the ``/uptime`` topic, and subscribes to the ``/number`` topic.
See the `MQTT-SN v1.2 specification`_ for more information.

The gateway address can either be configured statically, or discovered dynamically using the MQTT-SN Gateway Discovery procedure, selected with :option:`CONFIG_NET_SAMPLE_MQTT_SN_STATIC_GATEWAY`.
If the connection to the gateway is later lost, the sample reconnects automatically with an exponential backoff, giving up after a configurable number of attempts (see `Configuration options`_).

The sample itself has no output on success beyond the logging described in `Sample output`_.
Verify its functionality by observing the gateway and broker logs, or by subscribing to the sample's topics from another MQTT client.

Configuration
*************

|config|

Configuration options
======================

The following sample-specific Kconfig options are used in this sample (located in :file:`zephyr/samples/net/mqtt_sn_publisher/Kconfig`):

* :option:`CONFIG_NET_SAMPLE_MQTT_SN_STATIC_GATEWAY` - Selects a statically configured gateway address instead of Gateway Discovery.
* :option:`CONFIG_NET_SAMPLE_MQTT_SN_GATEWAY_ADDRESS` - IP address and port of the MQTT-SN gateway, used when the gateway is statically configured.
* :option:`CONFIG_NET_SAMPLE_MQTT_SN_BROADCAST_ADDRESS` - IP address and port used to broadcast Gateway Discovery and topic registration messages.
* :option:`CONFIG_NET_SAMPLE_MQTT_SN_BUFFER_SIZE` - Size of the TX and RX buffers used by the MQTT-SN client.
* :option:`CONFIG_NET_SAMPLE_MQTT_SN_RECONNECT_INITIAL_BACKOFF_MSEC` - Delay before the first reconnect attempt, in milliseconds.
* :option:`CONFIG_NET_SAMPLE_MQTT_SN_RECONNECT_MAX_BACKOFF_MSEC` - Upper bound on the delay between reconnect attempts, in milliseconds.
* :option:`CONFIG_NET_SAMPLE_MQTT_SN_RECONNECT_MAX_ATTEMPTS` - Number of reconnect attempts before the sample gives up.

Configuration files
====================

The sample provides a predefined configuration file, located in :file:`zephyr/samples/net/mqtt_sn_publisher`:

* :file:`prj.conf` - Default configuration file, configured for a statically defined gateway.

To add a specific extra configuration file to the build, add the ``-DEXTRA_CONF_FILE=<extra_conf_file>`` flag to your west build command.

Wi-Fi
=====

Use the :ref:`Wi-Fi snippet <zephyr:snippet-wifi-ipv4>` to build and run the sample on the development kits listed above.

Building and running
********************

.. |sample path| replace:: :file:`samples/zephyr/net/mqtt_sn_publisher`

.. include:: /includes/build_and_run.txt

Use the ``nrf7002dk/nrf5340/cpuapp`` or ``nrf7120dk/nrf7120/cpuapp`` board target together with the ``wifi-ipv4`` snippet.
For example:

.. code-block:: console

   # nRF7002 DK
   west build -b nrf7002dk/nrf5340/cpuapp -S wifi-ipv4

   # nRF7120 DK
   west build -b nrf7120dk/nrf7120/cpuapp -S wifi-ipv4

Alternatively, build against the predefined test case in :file:`sample.yaml`:

.. code-block:: console

   west build -b nrf7120dk/nrf7120/cpuapp -T nrf.extended.sample.net.mqtt_sn_publisher.wifi.nrf71dk

Before building, edit :file:`prj.conf` to set :option:`CONFIG_NET_SAMPLE_MQTT_SN_GATEWAY_ADDRESS` and :option:`CONFIG_NET_SAMPLE_MQTT_SN_BROADCAST_ADDRESS` to match your network, or set up an MQTT-SN gateway reachable at the addresses already configured there.

Testing
=======

|test_sample|

Testing this sample requires an MQTT-SN gateway and an MQTT broker reachable from the development kit's Wi-Fi network.
Follow Zephyr's :zephyr:code-sample:`mqtt-sn-publisher` sample documentation to set up `Mosquitto`_ and the `Eclipse Paho MQTT-SN Gateway`_ - the same setup works unchanged, since the gateway and broker only need to be reachable over IP.
Unlike the ``native_sim`` setup described there, set :option:`CONFIG_NET_SAMPLE_MQTT_SN_GATEWAY_ADDRESS` and :option:`CONFIG_NET_SAMPLE_MQTT_SN_BROADCAST_ADDRESS` in :file:`prj.conf` to addresses on your actual Wi-Fi network instead of the ``192.0.2.x`` defaults, matching wherever you run the gateway.

.. note::
   Mosquitto 2.x rejects anonymous clients by default once a listener is configured.
   Add ``allow_anonymous true`` to your :file:`mosquitto.conf`, or start it with ``mosquitto -v -p 1883 -c /dev/null`` to use the built-in defaults instead.

#. |connect_kit|
#. |connect_terminal|
#. Wait for the sample to connect to the configured Wi-Fi network and to the MQTT-SN gateway.
#. Observe the ``EVT_CONNECTED`` log line, followed by periodic ``Publishing timestamp`` lines.
#. From a host on the same network, subscribe to the ``/uptime`` topic to see the published values:

   .. code-block:: console

      mosquitto_sub -h <broker_host_ip_address> -t /uptime

Sample output
==============

This is a typical log output of the sample::

   [00:00:26.508,590] <inf> mqtt_sn_publisher_sample: Connecting client
   [00:00:27.008,705] <inf> net_mqtt_sn: MQTT_SN client connected
   [00:00:27.008,708] <inf> mqtt_sn_publisher_sample: MQTT-SN event EVT_CONNECTED
   [00:00:27.008,722] <inf> mqtt_sn_publisher_sample: Publishing timestamp
   [00:00:27.008,862] <inf> net_mqtt_sn: Registering topic
                                         2f 75 70 74 69 6d 65                             |/uptime
   [00:00:27.008,940] <inf> net_mqtt_sn: Can't publish; topic is not ready
   [00:00:27.508,883] <inf> net_mqtt_sn: Publishing to topic ID 2
   [00:00:37.010,461] <inf> mqtt_sn_publisher_sample: Publishing timestamp
   [00:00:37.010,514] <inf> net_mqtt_sn: Publishing to topic ID 2
   [...]
   [00:01:26.519,023] <inf> mqtt_sn_publisher_sample: MQTT-SN event EVT_PINGRESP
   [...]
   [00:03:16.509,607] <wrn> net_mqtt_sn: Ping ran out of retries
   [00:03:16.509,614] <inf> mqtt_sn_publisher_sample: MQTT-SN event EVT_DISCONNECTED
   [00:03:47.043,265] <inf> net_mqtt_sn: MQTT_SN client connected
   [00:03:47.043,267] <inf> mqtt_sn_publisher_sample: MQTT-SN event EVT_CONNECTED
   [...]
   [00:05:27.060,501] <inf> mqtt_sn_publisher_sample: MQTT-SN event EVT_PUBLISH
   [00:05:27.060,509] <inf> mqtt_sn_publisher_sample: Published data
                                                      34 32                                            |42

The gateway went silent (``Ping ran out of retries``), and the client reconnected on its own. ``EVT_PUBLISH`` / ``Published data`` is a message arriving on the subscribed ``/number`` topic.

Troubleshooting
================

If the sample does not connect to the Wi-Fi network, see the :ref:`wifi_monitor_sample` sample documentation to learn how to capture and analyze Wi-Fi traffic to debug connectivity issues.
Also verify that the Wi-Fi credentials configured for your device match your access point.

If the sample connects to Wi-Fi but never reaches the ``EVT_CONNECTED`` state, verify that the configured gateway and broadcast addresses are reachable from the development kit, and that the gateway and broker are both running.

References
**********

* `MQTT-SN v1.2 specification`_
* `Eclipse Paho MQTT-SN Gateway`_
* `Mosquitto`_

.. _`MQTT-SN v1.2 specification`: https://www.oasis-open.org/committees/download.php/66091/MQTT-SN_spec_v1.2.pdf
.. _`Eclipse Paho MQTT-SN Gateway`: https://www.eclipse.org/paho/index.php?page=components/mqtt-sn-transparent-gateway/index.php

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* MQTT-SN (:file:`include/zephyr/net/mqtt_sn.h`)
