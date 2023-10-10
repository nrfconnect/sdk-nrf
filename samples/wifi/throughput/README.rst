.. _wifi_throughput_sample:

Wi-Fi: Throughput
#################

.. contents::
   :local:
   :depth: 2

The Throughput sample provides a framework for users to measure the achievable IP networking throughput of applications using Nordic Semiconductor's Wi-Fi® chipsets.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how to measure the network throughput of a Nordic Wi-Fi-enabled platform using the **zperf** sample application under the different Wi-Fi stack configuration profiles.
The sample requires additional software, such as the Wi-Fi **iperf** application.
When the sample runs Wi-Fi UDP/TCP throughput in client mode, a peer device runs UDP/TCP throughput in server mode.
When the sample runs Wi-Fi UDP/TCP throughput in server mode, a peer device runs UDP/TCP throughput in client mode.

See the :ref:`nRF70_nRF5340_constrained_host` page for different Wi-Fi stack configuration profiles.
See `Network Traffic Generator`_ for instructions on how to use **zperf** and **iperf** applications.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_throughput_test_setup.png
     :alt: Wi-Fi Throughput sample test setup

     Wi-Fi Throughput sample test setup

The reference test setup shows the connections between the following devices:

* The nRF7002 DK on which the throughput sample runs the **zperf** application.
* Wi-Fi peer device that runs the **iperf** application.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/throughput`

.. include:: /includes/build_and_run_ns.txt

.. note::
   The sample is supported on the nRF7002 DK with QSPI as the interface between the nRF5340 host and the nRF7002 device.

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

To build for the nRF7002 DK with different profiles for Station mode, use the following CLI commands:

.. tabs::

   .. group-tab:: IoT devices

      To build for the nRF7002 DK, with the IoT device profile for Station mode, use the ``iot-devices`` overlay configuration.

      .. code-block:: console

         west build -p -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-iot-devices.conf

   .. group-tab:: Memory-optimized

      To build for the nRF7002 DK, with the memory-optimized profile for Station mode, use the ``memory-optimized`` overlay configuration.

      .. code-block:: console

         west build -p -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-memory-optimized.conf

   .. group-tab:: High performance

      To build for the nRF7002 DK, with the high performance profile for Station mode, use the ``high-performance`` overlay configuration.

      .. code-block:: console

         west build -p -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-high-performance.conf

   .. group-tab:: TX prioritized

      To build for the nRF7002 DK, with the TX prioritized profile for Station mode, use the ``tx-prio`` overlay configuration.

      .. code-block:: console

         west build -p -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-tx-prio.conf

   .. group-tab:: RX prioritized

      To build for the nRF7002 DK, with the RX prioritized profile for Station mode, use the ``rx-prio`` overlay configuration.

      .. code-block:: console

         west build -p -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-rx-prio.conf

Supported CLI commands
======================

``zperf`` is the command line tool and supports the following UART CLI subcommands:

.. list-table:: ``zperf`` shell subcommands
   :header-rows: 1

   * - Subcommands
     - Description
   * - connectap
     - | Connect to AP
   * - setip
     - | Set IP address
       | <my ip> <prefix len>
       | Example - setip 2001:db8::2 64
       | Example - setip 192.0.2.2
   * - tcp
     - | Upload/Download TCP data
   * - udp
     - | Upload/Download UDP data
   * - version
     - | **zperf** version

The following is an example of the ``zperf`` command:

.. code-block:: console

   zperf <udp/tcp> <upload/download> <dest ip> [<dest port> <duration> <packet size>[K] <baud rate>[K|M]]

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Scan for the Wi-Fi networks in range using the following command:

   .. code-block:: console

      wifi scan

   The output should be similar to the following:

   .. code-block:: console

      Scan requested

      Num  | SSID                             (len) | Chan (Band)   | RSSI | Security        | BSSID             | MFP
      1    | abcd                             8     | 11   (2.4GHz) | -18  | WPA2-PSK        | xx:xx:xx:xx:xx:xx | Disable
      2    | efgh                             7     | 40   (5GHz  ) | -21  | OPEN            | xx:xx:xx:xx:xx:xx | Disable
      3    | mnopq                            8     | 6    (2.4GHz) | -29  | OPEN            | xx:xx:xx:xx:xx:xx | Disable
      6    | stuvwx                           24    | 11   (2.4GHz) | -47  | WPA3-SAE        | xx:xx:xx:xx:xx:xx | Required

#. Connect to your preferred network using the following command:

   .. code-block:: console

      wifi connect <SSID> <passphrase>

   ``<SSID>`` is the SSID of the network you want to connect to, and ``<passphrase>`` is its passphrase.

#. Check the connection status after 30 seconds, using the following command:

   .. code-block:: console

      wifi status

   If the connection is established, you should see an output similar to the following:

   .. code-block:: console

      Status: successful
      ==================
      State: COMPLETED
      Interface Mode: STATION
      Link Mode: WIFI 4 (802.11n/HT)
      SSID: xxxxx
      BSSID: xx:xx:xx:xx:xx:xx
      Band: 2.4GHz
      Channel: 11
      Security: WPA2-PSK
      MFP: Optional
      RSSI: -28
      Beacon Interval: 100
      DTIM: 1
      TWT: Not supported

#. Initiate a ping and verify data connectivity using the following commands:

   .. code-block:: console

      net ping <resolved hostname>

   See the following example:

   .. code-block:: console

      net ping 192.168.1.1
      PING 192.168.1.1
      28 bytes from 192.168.1.1 to 192.168.1.1: icmp_seq=1 ttl=64 time=0 ms
      28 bytes from 192.168.1.1 to 192.168.1.1: icmp_seq=2 ttl=64 time=0 ms
      28 bytes from 192.168.1.1 to 192.168.1.1: icmp_seq=3 ttl=64 time=0 ms

#. Run the ``iperf`` commands (TX) on the peer device and the ``zperf`` commands (RX) on the sample as shown in the following table.

   +--------------+------------------+------------------------------------------------------------------------------------+
   | Traffic type | TX (Peer device) |                             RX (Sample)                                            |
   +==============+==================+====================================================================================+
   | UDP          | iperf -s -i 1 -u | zperf udp upload <dest ip> <dest port> <duration> <packet size> <baud rate>        |
   +--------------+------------------+------------------------------------------------------------------------------------+
   | TCP          | iperf -s -i 1    | zperf tcp upload <dest ip> <dest port> <duration> <packet size> <baud rate>        |
   +--------------+------------------+------------------------------------------------------------------------------------+

   The sample shows the following output (``zperf`` TX):

   .. code-block:: console

      $ zperf udp upload 192.168.1.253 5001 10 1K 50M
      Remote port is 5001
      Invalid IPv6 address 192.168.1.253
      Connecting to 192.168.1.253
      Duration:       10.00 s
      Packet size:    1024 bytes
      Rate:           51200 kbps
      Starting...
      Rate:           50.00 Mbps
      Packet duration 156 us
      -
      Upload completed!
      LAST PACKET NOT RECEIVED!!!
      Statistics:             server  (client)
      Duration:               29.45 m (10.00 s)
      Num packets:            0       (37440)
      Num packets out order:  25711
      Num packets lost:       4657
      Jitter:                 51.33 m
      Rate:                   0 Kbps  (29.24 Mbps)


#. Run the ``iperf`` commands (RX) on the peer device and the ``zperf`` commands (TX) on the sample as shown in the following table.

   +--------------+--------------------------------+------------------------------------------------------------+
   | Traffic type | TX (Sample)                    |                             RX (Peer device)               |
   +==============+================================+============================================================+
   | UDP          | zperf udp download <dest port> | iperf -c <ipaddress> -i 1 -t <duration> -u -b <baud rate>  |
   +--------------+--------------------------------+------------------------------------------------------------+
   | TCP          | zperf tcp download <dest port> | iperf -c <ipaddress> -i 1 -t <duration> -b <baud rate>     |
   +--------------+--------------------------------+------------------------------------------------------------+


   The sample shows the following output:

   .. code-block:: console

      $ zperf udp download 5001
      UDP server started on port 5001
      <inf> net_zperf: Binding to 192.168.1.13
      <inf> net_zperf: Binding to ::
      <inf> net_zperf: Listening on port 5001
      New session started.
      End of session!
      duration:              10.02 s
      received packets:      10694
      nb packets lost:       33884
      nb packets outorder:   0
      jitter:                        5.20 ms
      rate:                  11.96 Mbps

#. Stop the ``zperf`` UDP/TCP download using the following command:

   .. code-block:: console

      zperf <udp/tcp> download stop

   The sample shows the following output:

   .. code-block:: console

      $ zperf udp download stop
      UDP server stopped


Results
=======

The following table collects a summary of results obtained when throughput tests are run for different profiles.


+------------------------+-----------+-----------+-----------+-----------+
| Profile                | UDP TX    | UDP RX    | TCP TX    | TCP RX    |
|                        |           |           |           |           |
+========================+===========+===========+===========+===========+
| IoT devices            | 5.08 Mbps | 5.43 Mbps | 5.25 Mbps | 3.35 Mbps |
+------------------------+-----------+-----------+-----------+-----------+
| Memory-optimized       | 5.1 Mbps  | 113 Kbps  | 644 Kbps  | 1.86 Mbps |
+------------------------+-----------+-----------+-----------+-----------+
| High performance       | 26.2 Mbps | 13.08 Mbps| 14.1 Mbps | 7.76 Mbps |
+------------------------+-----------+-----------+-----------+-----------+
| TX prioritized         | 26.3 Mbps | 12.20 Mbps| 10.5 Mbps | 4.5 Mbps  |
+------------------------+-----------+-----------+-----------+-----------+
| RX prioritized         | 9.34 Mbps | 13.46 Mbps| 5.48 Mbps | 7.92 Mbps |
+------------------------+-----------+-----------+-----------+-----------+

.. note::
   As shown in the table above, the measured throughputs are based on tests conducted using the nRF7002 DK.
   The results represent the best throughput, averaged over three iterations, and were obtained with a good RSSI signal in a clean environment(RF Chamber).

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`

This sample also uses modules found in the following locations in the |NCS| folder structure:

* :file:`modules/lib/hostap`
* :file:`modules/mbedtls`
