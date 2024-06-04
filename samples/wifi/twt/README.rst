.. _wifi_twt_sample:

Wi-Fi: TWT
##########

.. contents::
   :local:
   :depth: 2

The TWT sample demonstrates how to use TWT power save feature.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample can perform Wi-Fi® operations such as connect and disconnect in the 2.4 GHz and 5 GHz bands depending on the capabilities of an access point.

Using this sample, the development kit can connect to the specified access point in :abbr:`STA (Station)` mode and setup TWT flow with the access point.
By default once the TWT flow is setup, the sample will use the traffic generator module to send and receive TCP data packets to and from the traffic generator server running on a PC connected to the same access point (typically through Ethernet).

Traffic generator server
************************

When the traffic generator module is used with this sample, the traffic generator server must be running on a PC connected to the same access point.
The server source is available in the :file:`scripts/traffic_gen_server` file.
This is a `python3` based server, so you must make sure that `python3` is installed on your PC.

To start the server:

1. Set the :kconfig:option:`CONFIG_TRAFFIC_GEN_REMOTE_IPV4_ADDR` Kconfig option to the IPv4 address of the host running the traffic generator server.

2. Run the following command:

   .. code-block:: console

      python3 traffic_gen_server.py

   Use the ``-h`` option to display the help message for how to use the server.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/twt/Kconfig`) :

.. options-from-kconfig::
   :show-type:

You must configure the following Wi-Fi credentials in the :file:`prj.conf` file:

.. include:: /includes/wifi_credentials_static.txt

.. note::
   You can also use ``menuconfig`` to configure ``Wi-Fi credentials``.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

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

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/twt`

.. include:: /includes/build_and_run_ns.txt

Currently, only the nRF7002 DK is supported.

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

    [00:00:02.016,235] <inf> twt: Connection requested
    [00:00:02.316,314] <inf> twt: ==================
    [00:00:02.316,314] <inf> twt: State: SCANNING
    [00:00:02.616,424] <inf> twt: ==================
    [00:00:02.616,424] <inf> twt: State: SCANNING
    [00:00:02.916,534] <inf> twt: ==================
    [00:00:02.916,534] <inf> twt: State: SCANNING
    [00:00:03.216,613] <inf> twt: ==================
    [00:00:03.216,613] <inf> twt: State: SCANNING
    [00:00:03.516,723] <inf> twt: ==================
    [00:00:03.516,723] <inf> twt: State: SCANNING
    [00:00:03.816,802] <inf> twt: ==================
    [00:00:03.816,802] <inf> twt: State: SCANNING
    [00:00:04.116,882] <inf> twt: ==================
    [00:00:04.116,882] <inf> twt: State: SCANNING
    [00:00:04.416,961] <inf> twt: ==================
    [00:00:04.416,961] <inf> twt: State: SCANNING
    [00:00:04.717,071] <inf> twt: ==================
    [00:00:04.717,071] <inf> twt: State: SCANNING
    [00:00:05.017,150] <inf> twt: ==================
    [00:00:05.017,150] <inf> twt: State: SCANNING
    [00:00:05.317,230] <inf> twt: ==================
    [00:00:05.317,230] <inf> twt: State: SCANNING
    [00:00:05.617,309] <inf> twt: ==================
    [00:00:05.617,309] <inf> twt: State: SCANNING
    [00:00:05.917,419] <inf> twt: ==================
    [00:00:05.917,419] <inf> twt: State: SCANNING
    [00:00:06.217,529] <inf> twt: ==================
    [00:00:06.217,529] <inf> twt: State: SCANNING
    [00:00:06.517,639] <inf> twt: ==================
    [00:00:06.517,639] <inf> twt: State: SCANNING
    [00:00:06.817,749] <inf> twt: ==================
    [00:00:06.817,749] <inf> twt: State: SCANNING
    [00:00:07.117,858] <inf> twt: ==================
    [00:00:07.117,858] <inf> twt: State: SCANNING
    [00:00:07.336,730] <inf> wpa_supp: wlan0: SME: Trying to authenticate with aa:bb:cc:dd:ee:ff (SSID='<MySSID>' freq=5785 MHz)
    [00:00:07.353,027] <inf> nrf_wifi: nrf_wifi_wpa_supp_authenticate:Authentication request sent successfully

    [00:00:07.417,938] <inf> twt: ==================
    [00:00:07.417,938] <inf> twt: State: AUTHENTICATING
    [00:00:07.606,628] <inf> wpa_supp: wlan0: Trying to associate with aa:bb:cc:dd:ee:ff (SSID='<MySSID>' freq=5785 MHz)
    [00:00:07.609,680] <inf> nrf_wifi: nrf_wifi_wpa_supp_associate: Association request sent successfully

    [00:00:07.621,978] <inf> wpa_supp: wpa_drv_zep_get_ssid: SSID size: 5

    [00:00:07.622,070] <inf> wpa_supp: wlan0: Associated with aa:bb:cc:dd:ee:ff
    [00:00:07.622,192] <inf> wpa_supp: wlan0: CTRL-EVENT-CONNECTED - Connection to aa:bb:cc:dd:ee:ff completed [id=0 id_str=]
    [00:00:07.622,192] <inf> twt: Connected
    [00:00:07.623,779] <inf> wpa_supp: wlan0: CTRL-EVENT-SUBNET-STATUS-UPDATE status=0
    [00:00:07.648,406] <inf> net_dhcpv4: Received: 192.168.119.6
    [00:00:07.648,468] <inf> net_config: IPv4 address: 192.168.119.6
    [00:00:07.648,498] <inf> net_config: Lease time: 3599 seconds
    [00:00:07.648,498] <inf> net_config: Subnet: 255.255.255.0
    [00:00:07.648,529] <inf> net_config: Router: 192.168.119.147
    [00:00:07.648,559] <inf> twt: DHCP IP address: 192.168.119.6
    [00:00:07.720,153] <inf> twt: ==================
    [00:00:07.720,153] <inf> twt: State: COMPLETED
    [00:00:07.720,153] <inf> twt: Interface Mode: STATION
    [00:00:07.720,184] <inf> twt: Link Mode: WIFI 6 (802.11ax/HE)
    [00:00:07.720,184] <inf> twt: SSID: <MySSID>
    [00:00:07.720,214] <inf> twt: BSSID: aa:bb:cc:dd:ee:ff
    [00:00:07.720,214] <inf> twt: Band: 5GHz
    [00:00:07.720,214] <inf> twt: Channel: 157
    [00:00:07.720,245] <inf> twt: Security: OPEN
    [00:00:07.720,245] <inf> twt: MFP: UNKNOWN
    [00:00:07.720,245] <inf> twt: RSSI: -57
    [00:00:07.720,245] <inf> twt: Static IP address:
    [00:01:14.217,224] <inf> twt: == TWT negotiated parameters ==
    [00:01:14.217,224] <inf> twt: TWT Dialog token: 1
    [00:01:14.217,224] <inf> twt: TWT flow ID: 1
    [00:01:14.217,254] <inf> twt: TWT negotiation type: TWT individual negotiation
    [00:01:14.217,285] <inf> twt: TWT responder: true
    [00:01:14.217,315] <inf> twt: TWT implicit: true
    [00:01:14.217,315] <inf> twt: TWT announce: true
    [00:01:14.217,346] <inf> twt: TWT trigger: true
    [00:01:14.217,376] <inf> twt: TWT wake interval: 65024 us
    [00:01:14.217,376] <inf> twt: TWT interval: 524000 us
    [00:01:14.217,376] <inf> twt: ========================
    [00:01:14.439,270] <inf> twt: TWT Setup Success
    [00:00:15.230,773] <inf> traffic_gen: Sending TCP uplink Traffic
    [00:00:45.846,374] <inf> traffic_gen: Sent TCP uplink traffic for 30 sec
    [00:00:45.501,892] <inf> traffic_gen: Server Report:
    [00:00:45.501,922] <inf> traffic_gen:    Total Bytes Received  : 904192
    [00:00:45.501,922] <inf> traffic_gen:    Total Packets Received: 1045
    [00:00:45.501,922] <inf> traffic_gen:    Elapsed Time          : 33
    [00:00:45.501,953] <inf> traffic_gen:    Throughput (Kbps)     : 214
    [00:00:45.501,953] <inf> traffic_gen:    Average Jitter (ms)   : 0
    [00:00:45.061,767] <inf> twt: TWT teardown success

Power management testing
************************

You can use this sample to measure the current consumption of both the nRF5340 SoC and the nRF7002 device independently by using two separate Power Profiler Kit II (PPK2) devices.
The nRF5340 SoC is connected to the first PPK2 and the nRF7002 DK is connected to the second PPK2.

See `Measuring current`_ for more information about how to set up and measure the current consumption of both the nRF5340 SoC and the nRF7002 device.

The average current consumption in an idle case can be around ~1-2 mA in the nRF5340 SoC and ~20 µA in the nRF7002 device.

See :ref:`app_power_opt` for more information on power management testing and usage of the PPK2.

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
