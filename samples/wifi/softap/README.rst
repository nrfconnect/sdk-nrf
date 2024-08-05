.. _wifi_softap_sample:

Wi-Fi: SoftAP
#############

.. contents::
   :local:
   :depth: 2

The SoftAP sample demonstrates how to start an nRF70 Series device in :term:`Software-enabled Access Point (SoftAP or SAP)` mode.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample enables SoftAP mode and allows the devices to connect to it.
It starts a Dynamic Host Configuration Protocol (DHCP) server to assign the automatic IP address to the connected devices.
Then, the sample lists the connected devices.

You can enable the SoftAP mode by setting the below configuration options in the sample project configuration file:

* :kconfig:option:`CONFIG_NRF70_AP_MODE`: Enables access point mode support.
* :kconfig:option:`CONFIG_WPA_SUPP_AP`: Enables access point support.

The sample uses the :ref:`lib_wifi_ready` library to check Wi-Fi readiness.
To use the :ref:`lib_wifi_ready` library, enable the :kconfig:option:`CONFIG_WIFI_READY_LIB` Kconfig option.

.. note::

   The SoftAP mode operation is dictated by regulatory requirements.
   It is mandatory to set the regulatory domain to a specific country when operating in the 5 GHz frequency band.

You can set the regulatory domain using the below configuration option:

* :kconfig:option:`CONFIG_SOFTAP_SAMPLE_REG_DOMAIN`: Sets the ISO/IEC alpha2 country code.

For more information, see the :ref:`nRF70_soft_ap_mode` documentation.

Configuration
*************

|config|

Configuration options
=====================

The following Kconfig options are used in this sample (located in :file:`samples/wifi/softap/Kconfig`):

.. options-from-kconfig::

IP addressing
*************

The sample starts the DHCP server on the SoftAP interface.
The station devices should use DHCP to get an IP address from the virtual router.

To specify the DHCP pool start address, you can edit the :kconfig:option:`CONFIG_SOFTAP_SAMPLE_DHCPV4_POOL_START` Kconfig option in the :file:`prj.conf` file.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/softap`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp

To build for the nRF7002 EK with nRF5340 DK, use the ``nrf5340dk/nrf5340/cpuapp`` board target with the ``SHIELD`` CMake option set to ``nrf7002ek``.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      *** Booting nRF Connect SDK v3.4.99-ncs1-4797-g7c3e830729b7 ***
      [00:00:00.580,596] <inf> net_config: Initializing network
      [00:00:00.580,596] <inf> net_config: Waiting interface 1 (0x200014e4) to be up...
      [00:00:00.580,718] <inf> net_config: IPv4 address: 192.168.1.1
      [00:00:00.581,268] <inf> wpa_supp: Successfully initialized wpa_supplicant
      [00:00:00.582,122] <inf> softap: Regulatory domain set to IN
      [00:00:00.602,508] <inf> softap: DHCPv4 server started and pool address start from 192.168.1.2
      [00:00:01.250,000] <inf> wpa_supp: wlan0: interface state UNINITIALIZED->ENABLED
      [00:00:01.250,061] <inf> wpa_supp: wlan0: AP-ENABLED
      [00:00:01.250,335] <inf> wpa_supp: wlan0: CTRL-EVENT-CONNECTED - Connection to nn:oo:rr:dd:ii:cc completed [id=0 id_str=]
      [00:00:01.254,608] <inf> softap: AP enable requested
      [00:00:01.256,225] <inf> softap: AP mode enabled
      [00:00:01.257,873] <inf> softap: Status: successful
      [00:00:01.257,904] <inf> softap: ==================
      [00:00:01.257,904] <inf> softap: State: COMPLETED
      [00:00:01.257,934] <inf> softap: Interface Mode: ACCESS POINT
      [00:00:01.257,965] <inf> softap: Link Mode: UNKNOWN
      [00:00:01.257,995] <inf> softap: SSID: MySoftAP
      [00:00:01.258,026] <inf> softap: BSSID: NN:OO:RR:DD:II:CC
      [00:00:01.258,026] <inf> softap: Band: 2.4GHz
      [00:00:01.258,056] <inf> softap: Channel: 6
      [00:00:01.258,056] <inf> softap: Security: OPEN
      [00:00:01.258,087] <inf> softap: MFP: Disable
      [00:00:01.258,087] <inf> softap: Beacon Interval: 100
      [00:00:01.258,087] <inf> softap: DTIM: 2
      [00:00:01.258,117] <inf> softap: TWT: Not supported
      [00:00:01.341,674] <inf> net_config: IPv6 address: fe80::f6ce:36ff:fe00:10d4
      [00:00:15.270,446] <inf> wpa_supp: HT: Forty MHz Intolerant is set by STA aa:bb:cc:dd:ee:ff in Association Request
      [00:00:15.286,376] <inf> wpa_supp: wlan0: AP-STA-CONNECTED aa:bb:cc:dd:ee:ff
      [00:00:15.286,529] <inf> softap: Station connected: AA:BB:CC:DD:EE:FF
      [00:00:15.286,529] <inf> softap: AP stations:
      [00:00:15.286,529] <inf> softap: ============
      [00:00:15.286,560] <inf> softap: Station 1:
      [00:00:15.286,560] <inf> softap: ==========
      [00:00:15.286,590] <inf> softap: MAC: AA:BB:CC:DD:EE:FF
      [00:00:15.286,590] <inf> softap: Link mode: WIFI 4 (802.11n/HT)
      [00:00:15.286,621] <inf> softap: TWT: Not supported

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nrf_security`
* :ref:`lib_wifi_ready`
