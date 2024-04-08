.. _wifi_promiscuous_sample:

Wi-Fi: Promiscuous
##################

.. contents::
   :local:
   :depth: 2

The Promiscuous sample demonstrates how to set Promiscuous mode, establish a connection to an Access Point (AP), analyze incoming Wi-Fi® packets, and print packet statistics.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how to configure the nRF70 Series device in Promiscuous mode, and how to connect the Wi-Fi station to a specified access point using the Dynamic Host Configuration Protocol (DHCP).
It analyzes the incoming Wi-Fi packets on a raw socket and prints the packet statistics at a fixed interval.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/promiscuous/Kconfig`):

.. options-from-kconfig::
   :show-type:

You must configure the following Wi-Fi credentials in the :file:`prj.conf` file:

.. include:: /includes/wifi_credentials_static.txt

.. note::
   You can also use ``menuconfig`` to configure ``Wi-Fi credentials``.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/promiscuous`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp

Change the board target as given below for the nRF7002 EK.

.. code-block:: console

   nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      [00:00:00.433,898] <inf> usb_net: netusb initialized
      *** Booting nRF Connect SDK v2.6.99-22ce705894ad ***
      *** Using Zephyr OS v3.6.99-688107b4a1a2 ***
      [00:00:00.516,448] <inf> net_config: Initializing network
      [00:00:00.516,448] <inf> net_config: Waiting interface 1 (0x20001020) to be up...
      [00:00:00.516,601] <inf> net_config: IPv4 address: 192.168.1.99
      [00:00:00.516,662] <inf> net_config: Running dhcpv4 client...
      [00:00:00.519,897] <inf> promiscuous: Promiscuous mode enabled
      [00:00:00.942,535] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000c not supported
      [00:00:00.991,516] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
      [00:00:00.993,713] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
      [00:00:00.994,537] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
      [00:00:00.994,995] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
      [00:00:00.996,520] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
      [00:00:03.520,324] <inf> wifi_connect: Static IP address (overridable): 192.168.1.99/255.255.255.0 -> 192.168.1.1
      [00:00:03.529,510] <inf> wifi_mgmt_ext: Connection requested
      [00:00:03.529,541] <inf> wifi_connect: Connection requested
      [00:00:07.712,585] <inf> wifi_connect: Connected
      [00:00:07.839,050] <inf> net_dhcpv4: Received: 192.165.100.161
      [00:00:07.839,202] <inf> net_config: IPv4 address: 192.165.100.161
      [00:00:07.839,202] <inf> net_config: Lease time: 86400 seconds
      [00:00:07.839,233] <inf> net_config: Subnet: 255.255.0.0
      [00:00:07.839,294] <inf> net_config: Router: 192.165.100.68
      [00:00:07.839,385] <inf> wifi_connect: DHCP IP address: 192.165.100.161
      [00:00:07.842,468] <inf> wifi_connect: ==================
      [00:00:07.842,498] <inf> wifi_connect: State: COMPLETED
      [00:00:07.842,529] <inf> wifi_connect: Interface Mode: STATION
      [00:00:07.842,529] <inf> wifi_connect: Link Mode: WIFI 4 (802.11n/HT)
      [00:00:07.842,559] <inf> wifi_connect: SSID: AAAAAAAA
      [00:00:07.842,590] <inf> wifi_connect: BSSID: XX:XX:XX:XX:XX:XX
      [00:00:07.842,620] <inf> wifi_connect: Band: 2.4GHz
      [00:00:07.842,620] <inf> wifi_connect: Channel: 11
      [00:00:07.842,620] <inf> wifi_connect: Security: OPEN
      [00:00:07.842,651] <inf> wifi_connect: MFP: Disable
      [00:00:07.842,651] <inf> wifi_connect: RSSI: -57
      [00:00:07.842,681] <inf> wifi_connect: TWT: Not supported
      [00:00:07.844,024] <inf> promiscuous: Network capture of Wi-Fi traffic enabled
      [00:00:07.844,177] <inf> promiscuous: Wi-Fi promiscuous mode RX thread started
      [00:00:12.869,476] <inf> promiscuous: Management Frames:
      [00:00:12.869,506] <inf> promiscuous:   Beacon Count: 48
      [00:00:12.869,506] <inf> promiscuous:   Probe Request Count: 0
      [00:00:12.869,506] <inf> promiscuous:   Probe Response Count: 190
      [00:00:12.869,537] <inf> promiscuous:   Action Count: 0
      [00:00:12.869,537] <inf> promiscuous: Control Frames:
      [00:00:12.869,567] <inf> promiscuous:    ACK Count 0
      [00:00:12.869,567] <inf> promiscuous:    RTS Count 0
      [00:00:12.869,567] <inf> promiscuous:    CTS Count 0
      [00:00:12.869,567] <inf> promiscuous:    Block Ack Count 0
      [00:00:12.869,598] <inf> promiscuous:    Block Ack Req Count 0
      [00:00:12.869,628] <inf> promiscuous:    CF End Count 0
      [00:00:12.869,628] <inf> promiscuous: Data Frames:
      [00:00:12.869,628] <inf> promiscuous:   Data Count: 9
      [00:00:12.869,628] <inf> promiscuous:   QoS Data Count: 0
      [00:00:12.869,659] <inf> promiscuous:   Null Count: 16
      [00:00:12.869,659] <inf> promiscuous:   QoS Null Count: 8
      [00:00:12.869,659] <inf> promiscuous: Unknown Frames:
      [00:00:12.869,659] <inf> promiscuous:   Unknown Management Frame Count: 1
      [00:00:12.869,659] <inf> promiscuous:   Unknown Control Frame Count: 0
      [00:00:12.869,689] <inf> promiscuous:   Unknown Data Frame Count: 0
      [00:00:12.869,689] <inf> promiscuous:   Reserved Frame Count: 0
      [00:00:12.869,689] <inf> promiscuous:   Unknown Frame Count: 0

Offline net capture
*******************

.. include:: /includes/offline_net_capture.txt

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`nrf_security`
