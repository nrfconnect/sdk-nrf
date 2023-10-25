.. _wifi_raw_tx_packet_sample:

Wi-Fi: Raw TX packet
####################

.. contents::
   :local:
   :depth: 2

The Raw TX packet sample demonstrates how to transmit raw IEEE 802.11 packets.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********
The sample generates and broadcasts 802.11 beacon frames as raw TX packets.
As a consequence, the nRF70 Series device can be identified as a Wi-Fi® beaconing device.

The sample demonstrates how to transmit raw TX packets in both connected Station and non-connected Station modes of operation.
The sample provides the option to select the traffic pattern between the following modes:

* :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_TRANSMISSION_MODE_CONTINUOUS`: Selects continuous packet transmission.
* :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_TRANSMISSION_MODE_FIXED`: Selects fixed number of transmitted packets.

The configurations for connected Station or non-connected Station modes, and for continuous or fixed packet transmission, are set at build time.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/raw_tx_packet/Kconfig`):

.. options-from-kconfig::

Configuration options for operating modes
-----------------------------------------

By using the following Kconfig options, you can configure the sample for different operational modes:

* For connected Station mode

  To configure the sample in connected Station mode, you must configure the following Wi-Fi credentials in the :file:`prj.conf` file:

  * :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_SSID`: Sets the name of your Wi-Fi network.
  * :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_KEY_MGMT_NONE`: Selects open security.
  * :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_KEY_MGMT_WPA2`: Selects PSK security.
  * :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_KEY_MGMT_WPA2_256`: Selects PSK-256 security.
  * :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_KEY_MGMT_WPA3`: Selects SAE security (default).
  * :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_PASSWORD`: Sets the password of your Wi-Fi network.

  .. note::
     You can also use ``menuconfig`` to enable the ``Key management`` option.

  See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

* For non-connected Station mode

  To configure the sample in non-connected Station mode, you must configure the :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_CHANNEL` Kconfig option in the :file:`prj.conf` file.

  This specifies the Wi-Fi channel to be used for communication on the wireless network.

Configuration options for raw TX packet header
----------------------------------------------

The following configuration options are available for the raw TX packet header:

* :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_RATE_VALUE`: Specifies the data transmission PHY rate.
* :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_RATE_FLAGS`: Specifies the data transmission mode.
* :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_QUEUE_NUM`: Specifies the transmission queue to which raw TX packets are assigned for sending.

Additionally, you must configure the :kconfig:option:`CONFIG_RAW_TX_PACKET_SAMPLE_INTER_FRAME_DELAY_MS` Kconfig option in the :file:`prj.conf` file to define the time delay between raw TX packets.

This sets the time duration between raw TX packets.

IP addressing
*************

The sample uses DHCP to obtain an IP address for the Wi-Fi interface.
It starts with a default static IP address to handle networks without DHCP servers, or if the DHCP server is not available.
Successful DHCP handshake will override the default static IP configuration.

You can change the following default static configuration in the :file:`prj.conf` file:

.. code-block:: console

  CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.168.1.98"
  CONFIG_NET_CONFIG_MY_IPV4_NETMASK="255.255.255.0"
  CONFIG_NET_CONFIG_MY_IPV4_GW="192.168.1.1"

.. note::

   This section is specific to the connected Station mode.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/raw_tx_packet`

.. include:: /includes/build_and_run_ns.txt

The sample can be built for the following configurations:

* Continuous raw 802.11 packet transmission in the connected Station mode.
* Fixed number of raw 802.11 packet transmission in the connected Station mode.
* Continuous raw 802.11 packet transmission in the non-connected Station mode.
* Fixed number of raw 802.11 packet transmission in the non-connected Station mode.

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following are examples of the CLI commands:

* Continuous raw 802.11 packet transmission in the connected Station mode:

  .. code-block:: console

     west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_RAW_TX_PACKET_SAMPLE_CONNECTION_MODE=y -DCONFIG_RAW_TX_PACKET_SAMPLE_TRANSMISSION_MODE_CONTINUOUS=y

* Fixed number of raw 802.11 packet transmission in the connected Station mode:

  .. code-block:: console

     west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_RAW_TX_PACKET_SAMPLE_CONNECTION_MODE=y -DCONFIG_RAW_TX_PACKET_SAMPLE_TRANSMISSION_MODE_FIXED=y -DCONFIG_RAW_TX_PACKET_SAMPLE_FIXED_NUM_PACKETS=<number of packets to be sent>

* Continuous raw 802.11 packet transmission in the non-connected Station mode:

  .. code-block:: console

     west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_RAW_TX_PACKET_SAMPLE_NON_CONNECTED_MODE=y -DCONFIG_RAW_TX_PACKET_SAMPLE_TRANSMISSION_MODE_CONTINUOUS=y

* Fixed number of raw 802.11 packet transmission in the non-connected Station mode:

  .. code-block:: console

     west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_RAW_TX_PACKET_SAMPLE_NON_CONNECTED_MODE=y -DCONFIG_RAW_TX_PACKET_SAMPLE_TRANSMISSION_MODE_FIXED=y -DCONFIG_RAW_TX_PACKET_SAMPLE_FIXED_NUM_PACKETS=<number of packets to be sent>

Change the build target as given below for the nRF7002 EK.

.. code-block:: console

   nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002ek

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      [00:00:00.469,940] <err> wifi_nrf: Firmware (v1.2.8.99) booted successfully

      *** Booting nRF Connect SDK 9a9ffb5ebb5b ***
      [00:00:00.618,713] <inf> net_config: Initializing network
      [00:00:00.618,713] <inf> net_config: Waiting interface 1 (0x20001570) to be up...
      [00:00:00.618,835] <inf> net_config: IPv4 address: 192.168.1.99
      [00:00:00.618,896] <inf> net_config: Running dhcpv4 client...
      [00:00:00.619,140] <inf> raw_tx_packet: Starting nrf7002dk_nrf5340_cpuapp with CPU frequency: 64 MHz
      [00:00:01.619,293] <inf> raw_tx_packet: Static IP address (overridable): 192.168.1.99/255.255.255.0 -> 192.168.1.1
      [00:00:01.632,507] <inf> raw_tx_packet: Wi-Fi channel set to 6
      [00:00:01.632,598] <inf> raw_tx_packet: Sending 25 number of raw tx packets
      [00:00:01.730,010] <inf> net_config: IPv6 address: fe80::f6ce:36ff:fe00:2282

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
