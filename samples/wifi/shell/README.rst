.. _wifi_shell_sample:

Wi-Fi: Shell
############

.. contents::
   :local:
   :depth: 2

This sample allows you to test Nordic Semiconductor's Wi-Fi® chipsets.

Requirements
************

The sample supports the following development kits:

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
* 9160 DK + SPIM


To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

To build for the nRF7002 EK with nRF5340 DK, use the ``nrf5340dk_nrf5340_cpuapp`` build target with the ``SHIELD`` CMake option set to ``nrf7002ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002ek

To build for the nRF9160 DK, use the ``nrf9160dk_nrf9160_ns`` build target with the ``SHIELD`` CMake option set to ``nrf7002ek`` and scan-only overlay configuration.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-scan-only.conf -DSHIELD=nrf7002ek

See also :ref:`cmake_options` for instructions on how to provide CMake options.

Supported CLI commands
======================

``wifi`` is the Wi-Fi command line and supports the following UART CLI subcommands:

.. list-table:: Wi-Fi shell subcommands
   :header-rows: 1

   * - Subcommands
     - Description
   * - scan
     - Scan for Wi-Fi APs
   * - connect
     - | Connect to a Wi-Fi AP with the following parameters:
       | <SSID>
       | <Channel number> (optional: 0 means all)
       | <PSK> (optional: valid only for secured SSIDs)
       | <Security type> (optional: 0-None, 1-PSK, 2-PSK-256, 3-SAE)
       | <MFP> (optional: 0-Disable, 1-Optional, 2-Required)
   * - disconnect
     - Disconnect from the Wi-Fi AP
   * - status
     - Status of the Wi-Fi interface
   * - statistics
     - Wi-Fi interface statistics
   * - ap
     - | Access Point mode commands
       | enable - Enable Access Point mode, with the following parameters:
       | <SSID>
       | <SSID length>
       | <channel> [optional]
       | <psk> [optional]
       | disable - Disable Access Point mode
       | (Note that the Access Point mode is presently not supported.)
   * - ps
     - | Configure power save
       | No argument - Prints current configuration
       | on - Turns on power save feature
       | off - Turns off power save feature
   * - ps_mode
     - | Configure Wi-Fi power save mode
       | 0 - Legacy
       | 1 - WMM
   * - twt
     - | Manage Target Wake Time (TWT) flows with below subcommands:
       |
       | setup - Start a TWT flow:
       | <negotiation_type: 0 - Individual, 1 - Broadcast, 2 - Wake TBTT>
       | <setup_cmd: 0 - Request, 1 - Suggest, 2 - Demand>
       | <dialog_token: 1-255> <flow_id: 0-7> <responder: 0/1> <trigger: 0/1>
       | <implicit: 0/1> <announce: 0/1> <twt_wake_interval: 1-262144 µs>
       | <twt_interval: 1µs-2^31µs>
       |
       | teardown - Teardown a TWT flow:
       | <negotiation_type: 0 - Individual, 1 - Broadcast, 2 - Wake TBTT>
       | <setup_cmd: 0 - Request, 1 - Suggest, 2 - Demand>
       | <dialog_token: 1-255> <flow_id: 0-7>
       |
       | teardown_all - Teardown all TWT flows
   * - reg_domain
     - | Set or get Wi-Fi regulatory domain
       |
       | Usage: wifi reg_domain [ISO/IEC 3166-1 alpha2] [-f]
       |
       | -f: Force to use this regulatory hint over any other regulatory hints.
       | (Note that this may cause regulatory compliance issues.)
   * - ps_timeout
     - | Configure Wi-Fi power save inactivity timer (in ms)
   * - ps_listen_interval
     - | Configure Wi-Fi power save for the Listen interval
       | <0-65535>
   * - ps_wakeup_mode
     - | Configure Wi-Fi power save for wakeup mode
       | dtim - Wakeup mode for the DTIM interval
       | listen_interval - Wakeup mode for the Listen interval

``wifi_cred`` is an extension to the Wi-Fi command line.
It adds the following commands to interact with the :ref:`lib_wifi_credentials` library:

.. list-table:: Wi-Fi credentials shell subcommands
   :header-rows: 1

   * - Subcommands
     - Description
   * - add
     - | Add a network to the credentials storage with following parameters:
       | <SSID>
       | <Passphrase> (optional: valid only for secured SSIDs)
       | <BSSID> (optional)
       | <Band> (optional: 2.4GHz, 5GHz)
       | favorite (optional, makes the network higher priority in automatic connection)
   * - delete <SSID>
     - Removes network from credentials storage.
   * - list
     - Lists networks in credential storage.
   * - auto_connect
     - Automatically connects to any stored network.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Scan for the Wi-Fi networks in range using the following command::

     wifi scan

   The output should be similar to the following::

      Scan requested

      Num  | SSID                             (len) | Chan (Band)    | RSSI | Security        | BSSID
      1    | xyza                             4     | 1    (2.4GHz)  | -27  | WPA2-PSK        | xx:xx:xx:xx:xx:xx
      2    | abcd                             4     | 149  (5GHz  )  | -28  | WPA2-PSK        | yy:yy:yy:yy:yy:yy



#. Connect to your preferred network using the following command::

     wifi connect <SSID> <passphrase>

   ``<SSID>`` is the SSID of the network you want to connect to, and ``<passphrase>`` is its passphrase.

#. Check the connection status after a while, using the following command::

     wifi status

   If the connection is established, you should see an output similar to the following::

      Status: successful
      ==================
      State: COMPLETED
      Interface Mode: STATION
      Link Mode: WIFI 6 (802.11ax/HE)
      SSID: OpenWrt
      BSSID: C0:06:C3:1D:CF:9E
      Band: 5GHz
      Channel: 157
      Security: WPA2-PSK
      PMF: Optional
      RSSI: 0

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
************

This sample uses the following library:

* :ref:`nrf_security`

This sample also uses modules found in the following locations in the |NCS| folder structure:

* :file:`modules/lib/hostap`
* :file:`modules/mbedtls`
