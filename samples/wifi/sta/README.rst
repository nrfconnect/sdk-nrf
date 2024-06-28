.. _wifi_station_sample:

Wi-Fi: Station
##############

.. contents::
   :local:
   :depth: 2

The Station sample demonstrates how to connect the Wi-Fi® station to a specified access point using Dynamic Host Configuration Protocol (DHCP).

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample can perform Wi-Fi operations such as connect and disconnect in the 2.4GHz and 5GHz bands depending on the capabilities of an access point.

Using this sample, the development kit can connect to the specified access point in :abbr:`STA (Station)` mode.

The sample uses the :ref:`lib_wifi_ready` library to check Wi-Fi readiness.
To use the :ref:`lib_wifi_ready` library, enable the :kconfig:option:`CONFIG_WIFI_READY_LIB` Kconfig option.

User interface
**************

The sample adds LED support to map with connection and disconnection events.

LED 1:
   Starts blinking when the sample is connected to the access point.

   Stops blinking when the sample is disconnected from the access point.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/sta/Kconfig`):

.. options-from-kconfig::
   :show-type:

You must configure the following Wi-Fi credentials in the :file:`prj.conf` file:

.. include:: /includes/wifi_credentials_static.txt

.. note::
   You can also use ``menuconfig`` to configure ``Wi-Fi credentials``.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Quad Serial Peripheral Interface (QSPI) encryption
**************************************************

This sample demonstrates QSPI encryption API usage.
You can set the key using the :kconfig:option:`CONFIG_NRF700X_QSPI_ENCRYPTION_KEY` Kconfig option.

If encryption of the QSPI traffic is required for the production devices, matching keys must be programmed in both the nRF7002 OTP and non-volatile storage associated with the host.
The key from non-volatile storage must be set as the encryption key using the APIs.

Power management
****************

This sample also enables Zephyr's power management policy by default, which sets the nRF5340 :term:`System on Chip (SoC)` into low-power mode whenever it is idle.
See :ref:`zephyr:pm-guide` in the Zephyr documentation for more information on power management.

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

.. |sample path| replace:: :file:`samples/wifi/sta`

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

    [00:00:02.016,235] <inf> sta: Connection requested
    [00:00:02.316,314] <inf> sta: ==================
    [00:00:02.316,314] <inf> sta: State: SCANNING
    [00:00:02.616,424] <inf> sta: ==================
    [00:00:02.616,424] <inf> sta: State: SCANNING
    [00:00:02.916,534] <inf> sta: ==================
    [00:00:02.916,534] <inf> sta: State: SCANNING
    [00:00:03.216,613] <inf> sta: ==================
    [00:00:03.216,613] <inf> sta: State: SCANNING
    [00:00:03.516,723] <inf> sta: ==================
    [00:00:03.516,723] <inf> sta: State: SCANNING
    [00:00:03.816,802] <inf> sta: ==================
    [00:00:03.816,802] <inf> sta: State: SCANNING
    [00:00:04.116,882] <inf> sta: ==================
    [00:00:04.116,882] <inf> sta: State: SCANNING
    [00:00:04.416,961] <inf> sta: ==================
    [00:00:04.416,961] <inf> sta: State: SCANNING
    [00:00:04.717,071] <inf> sta: ==================
    [00:00:04.717,071] <inf> sta: State: SCANNING
    [00:00:05.017,150] <inf> sta: ==================
    [00:00:05.017,150] <inf> sta: State: SCANNING
    [00:00:05.317,230] <inf> sta: ==================
    [00:00:05.317,230] <inf> sta: State: SCANNING
    [00:00:05.617,309] <inf> sta: ==================
    [00:00:05.617,309] <inf> sta: State: SCANNING
    [00:00:05.917,419] <inf> sta: ==================
    [00:00:05.917,419] <inf> sta: State: SCANNING
    [00:00:06.217,529] <inf> sta: ==================
    [00:00:06.217,529] <inf> sta: State: SCANNING
    [00:00:06.517,639] <inf> sta: ==================
    [00:00:06.517,639] <inf> sta: State: SCANNING
    [00:00:06.817,749] <inf> sta: ==================
    [00:00:06.817,749] <inf> sta: State: SCANNING
    [00:00:07.117,858] <inf> sta: ==================
    [00:00:07.117,858] <inf> sta: State: SCANNING
    [00:00:07.336,730] <inf> wpa_supp: wlan0: SME: Trying to authenticate with aa:bb:cc:dd:ee:ff (SSID='<MySSID>' freq=5785 MHz)
    [00:00:07.353,027] <inf> nrf_wifi: nrf_wifi_wpa_supp_authenticate:Authentication request sent successfully

    [00:00:07.417,938] <inf> sta: ==================
    [00:00:07.417,938] <inf> sta: State: AUTHENTICATING
    [00:00:07.606,628] <inf> wpa_supp: wlan0: Trying to associate with aa:bb:cc:dd:ee:ff (SSID='<MySSID>' freq=5785 MHz)
    [00:00:07.609,680] <inf> nrf_wifi: nrf_wifi_wpa_supp_associate: Association request sent successfully

    [00:00:07.621,978] <inf> wpa_supp: wpa_drv_zep_get_ssid: SSID size: 5

    [00:00:07.622,070] <inf> wpa_supp: wlan0: Associated with aa:bb:cc:dd:ee:ff
    [00:00:07.622,192] <inf> wpa_supp: wlan0: CTRL-EVENT-CONNECTED - Connection to aa:bb:cc:dd:ee:ff completed [id=0 id_str=]
    [00:00:07.622,192] <inf> sta: Connected
    [00:00:07.623,779] <inf> wpa_supp: wlan0: CTRL-EVENT-SUBNET-STATUS-UPDATE status=0
    [00:00:07.648,406] <inf> net_dhcpv4: Received: 192.168.119.6
    [00:00:07.648,468] <inf> net_config: IPv4 address: 192.168.119.6
    [00:00:07.648,498] <inf> net_config: Lease time: 3599 seconds
    [00:00:07.648,498] <inf> net_config: Subnet: 255.255.255.0
    [00:00:07.648,529] <inf> net_config: Router: 192.168.119.147
    [00:00:07.648,559] <inf> sta: DHCP IP address: 192.168.119.6
    [00:00:07.720,153] <inf> sta: ==================
    [00:00:07.720,153] <inf> sta: State: COMPLETED
    [00:00:07.720,153] <inf> sta: Interface Mode: STATION
    [00:00:07.720,184] <inf> sta: Link Mode: WIFI 6 (802.11ax/HE)
    [00:00:07.720,184] <inf> sta: SSID: <MySSID>
    [00:00:07.720,214] <inf> sta: BSSID: aa:bb:cc:dd:ee:ff
    [00:00:07.720,214] <inf> sta: Band: 5GHz
    [00:00:07.720,214] <inf> sta: Channel: 157
    [00:00:07.720,245] <inf> sta: Security: OPEN
    [00:00:07.720,245] <inf> sta: MFP: UNKNOWN
    [00:00:07.720,245] <inf> sta: RSSI: -57
    [00:00:07.720,245] <inf> sta: Static IP address:

RPU recovery
************

The RPU recovery mechanism is used to recover from the RPU (nRF70) hang.
This feature performs an interface reset (down and up), which triggers a RPU cold boot.
Application's network connection will be lost during the recovery process, and it is application's responsibility to reestablish the network connection.

Testing
=======

To test RPU recovery, you must build the sample with :kconfig:option:`CONFIG_SHELL` and :kconfig:option:`CONFIG_NRF700X_UTIL` Kconfig options.

#. Trigger RPU recovery using the following command:

   .. code-block:: console

      wifi_util rpu_recovery_test

   If RPU recovery is triggered, you should see an output similar to the following:

   .. code-block:: console

      RPU recovery triggered

Power management testing
************************

You can use this sample to measure the current consumption of both the nRF5340 SoC and the nRF7002 device independently by using two separate Power Profiler Kit II (PPK2) devices.
The nRF5340 SoC is connected to the first PPK2 and the nRF7002 DK is connected to the second PPK2.

See `Measuring current`_ for more information about how to set up and measure the current consumption of both the nRF5340 SoC and the nRF7002 device.

The average current consumption in an idle case can be around ~1-2 mA in the nRF5340 SoC and ~20 µA in the nRF7002 device.

See :ref:`app_power_opt` for more information on power management testing and usage of the PPK2.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nrf_security`
* :ref:`lib_wifi_ready`
* :ref:`lib_wifi_credentials`
