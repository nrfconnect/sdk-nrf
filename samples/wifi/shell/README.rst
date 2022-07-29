.. _wifi_shell:

Wi-Fi: Shell
############

.. contents::
   :local:
   :depth: 2

This sample allows you to test Nordic Semiconductor's Wi-Fi chipsets.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample can perform all Wi-Fi operations in the 2.4GHz and 5GHz bands depending on the capabilities supported in the underlying chipset.

Using this sample, the development kit can associate with, and ping to, any Wi-Fi capable access point in :abbr:`STA (Station)` mode.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/shell`

.. include:: /includes/build_and_run_ns.txt

Currently, the following configurations are supported:

* 7002 DK + QSPI
* 7002 EK + SPIM


To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command::

   west build -b nrf7002dk_nrf5340_cpuapp

To build for the nRF7002 EK, use the ``nrf7002dk_nrf5340_cpuapp`` build target with the ``SHIELD`` CMake option set to ``nrf7002_ek``.
The following is an example of the CLI command::

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002_ek

See also :ref:`cmake_options` for instructions on how to provide CMake options.

Supported CLI commands
======================

``wpa_cli`` is the Wi-Fi command line and supports the following UART CLI subcommands:

+-----------------+----------------------------------------------------------------------------+
|Subcommands      |Description                                                                 |
+=================+============================================================================+
|scan             |Scan AP                                                                     |
+-----------------+----------------------------------------------------------------------------+
|connect          |<SSID>                                                                      |
|                 |<Passphrase> (optional: valid only for secured SSIDs)                       |
|                 |<KEY_MGMT> (optional: 0-None, 1-WPA2, 2-WPA2-256, 3-WPA3)                   |
+-----------------+----------------------------------------------------------------------------+
|add_network      |Add Network network id/number                                               |
+-----------------+----------------------------------------------------------------------------+
|set_network      |Set Network params                                                          |
+-----------------+----------------------------------------------------------------------------+
|set              |Set Global params                                                           |
+-----------------+----------------------------------------------------------------------------+
|get              |Get Global params                                                           |
+-----------------+----------------------------------------------------------------------------+
|enable_network   |Enable Network                                                              |
+-----------------+----------------------------------------------------------------------------+
|remove_network   |Remove Network                                                              |
+-----------------+----------------------------------------------------------------------------+
|get_network      |Get Network                                                                 |
+-----------------+----------------------------------------------------------------------------+
|select_network   |Select Network which will be enabled; rest of the networks will be disabled |
+-----------------+----------------------------------------------------------------------------+
|disable_network  |                                                                            |
+-----------------+----------------------------------------------------------------------------+
|disconnect       |                                                                            |
+-----------------+----------------------------------------------------------------------------+
|reassociate      |                                                                            |
+-----------------+----------------------------------------------------------------------------+
|status           |Get client status                                                           |
+-----------------+----------------------------------------------------------------------------+
|bssid            |Associate with this BSSID                                                   |
+-----------------+----------------------------------------------------------------------------+
|sta_autoconnect  |                                                                            |
+-----------------+----------------------------------------------------------------------------+
|signal_poll      |                                                                            |
+-----------------+----------------------------------------------------------------------------+

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Scan for the Wi-FI networks in range using the following command::

     wpa_cli scan

   The output should be similar to the following::

      Scan requested
      Num  | SSID                             (len) | Chan | RSSI | Sec
      1    | dlink-7D64                       10    | 1    | 0    | WPA/WPA2
      2    | abcd_527_24                      11    | 4    | 0    | Open
      3    | ASUS_RTAX88U11g                  15    | 3    | 0    | WPA/WPA2
      4    | TP-Link_6BA8                     12    | 9    | 0    | WPA/WPA2

#. Connect to your preferred network using the following command::

     wpa_cli connect <SSID> <passphrase>

   ``<SSID>`` is the SSID of the network you want to connect to, and ``<passphrase>`` is its passphrase.

#. Check the connection status after a while, using the following command::

     wpa_cli status

   If the connection is established, you should see an output similar to the following::

      Reply: bssid=c0:06:c3:1d:cf:9e
      freq=0
      ssid=ASUS_RTAX88U11g
      id=0
      mode=station
      pairwise_cipher=CCMP
      group_cipher=CCMP
      key_mgmt=WPA2-PSK
      pmf=1
      mgmt_group_cipher=BIP
      wpa_state=COMPLETED
      ip_address=192.168.0.206
      address=f4:ce:36:00:00:16

#. Initiate a ping and verify data connectivity using the following commands::

     net dns <hostname>
     net ping <resolved hostname>

   See the following example::

     net dns google.com
      Query for 'google.com' sent.
      dns: 142.250.74.46
      dns: All results received

     net ping 10 142.250.74.46
      PING 142.250.74.46
      28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=0 ttl=113 time=191 ms
      28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=1 ttl=113 time=190 ms
      28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=2 ttl=113 time=190 ms

Dependencies
============

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_security`

This sample also uses modules that can be found in the following locations in the |NCS| folder structure:

* ``modules/lib/hostap``
* ``modules/mbedtls``
