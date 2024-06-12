.. _wifi_shell_sample:

Wi-Fi: Shell
############

.. contents::
   :local:
   :depth: 2

The Shell sample allows you to test Nordic Semiconductor's Wi-Fi® chipsets.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample can perform all Wi-Fi operations in the 2.4GHz and 5GHz bands depending on the capabilities supported in the underlying chipset.

Using this sample, the development kit can associate with, and ping to, any Wi-Fi capable access point in :abbr:`STA (Station)` mode.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/shell`

.. include:: /includes/build_and_run_ns.txt

Currently, the following configurations are supported:

* nRF7002 DK + QSPI
* nRF7002 EK + SPIM
* nRF91 Series DK + SPIM


To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp

To build for the nRF7002 EK with nRF5340 DK, use the ``nrf5340dk/nrf5340/cpuapp`` board target with the ``SHIELD`` CMake option set to ``nrf7002ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek

To build with ``raw_tx`` shell support for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target and raw TX overlay configuration.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=overlay-raw-tx.conf

.. tabs::

   .. group-tab:: nRF9161 DK

      To build for the nRF9161 DK, use the ``nrf9161dk/nrf9161/ns`` board target with the ``SHIELD`` CMake option set to ``nrf7002ek`` and a scan-only overlay configuration.
      The following is an example of the CLI command:

      .. code-block:: console

         west build -p -b nrf9161dk/nrf9161/ns -- -DEXTRA_CONF_FILE=overlay-scan-only.conf -DSHIELD=nrf7002ek

   .. group-tab:: nRF9160 DK

      To build for the nRF9160 DK, use the ``nrf9160dk/nrf9160/ns`` board target with the ``SHIELD`` CMake option set to ``nrf7002ek`` and a scan-only overlay configuration.
      The following is an example of the CLI command:

    .. code-block:: console

       west build -b nrf9160dk/nrf9160/ns -- -DEXTRA_CONF_FILE=overlay-scan-only.conf -DSHIELD=nrf7002ek

See also :ref:`cmake_options` for instructions on how to provide CMake options.

Supported CLI commands
======================

``wifi`` is the Wi-Fi command line and supports the following UART CLI subcommands:

.. list-table:: Wi-Fi shell subcommands
   :header-rows: 1

   * - Subcommands
     - Description
   * - scan
     - | Scan for Wi-Fi APs
       | OPTIONAL PARAMETERS:
       | [-t, --type <active/passive>] : Preferred mode of scan. The actual mode
       | of scan can depend on factors such as the Wi-Fi chip implementation,
       | regulatory domain restrictions. Default type is active.
       | [-b, --bands <Comma separated list of band values (2/5/6)>] : Bands to be
       | scanned where 2: 2.4 GHz, 5: 5 GHz, 6: 6 GHz.
       | [-a, --dwell_time_active <val_in_ms>] : Active scan dwell time (in ms) on
       | a channel. Range 5 ms to 1000 ms.
       | [-p, --dwell_time_passive <val_in_ms>] : Passive scan dwell time (in ms)
       | on a channel. Range 10 ms to 1000 ms.
       | [-s, --ssids <Comma separate list of SSIDs>] : SSID list to scan for.
       | [-m, --max_bss <val>] : Maximum BSSes to scan for. Range 1 - 65535.
       | [-c, --chans <Comma separated list of channel ranges>] : Channels to be
       | scanned. The channels must be specified in the form
       | band1:chan1,chan2_band2:chan3,..etc. band1, band2 must be valid band
       | values and chan1, chan2, chan3 must be specified as a list of comma
       | separated values where each value is either a single channel or a channel
       | range specified as chan_start-chan_end. Each band channel set has to be
       | separated by a _. For example, a valid channel specification can be
       | 2:1,6-11,14_5:36,149-165,44
       | [-h, --help] : Print out the help for the scan command.
   * - connect
     - | Connect to a Wi-Fi AP
       | <-s --ssid \"<SSID>\">: SSID.
       | [-c --channel]: Channel that needs to be scanned for connection. 0:any channel
       | [-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz)
       | [-p, --psk]: Passphrase (valid only for secure SSIDs)
       | [-k, --key-mgmt]: Key management type.
       | 0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE, 4:WAPI, 5:EAP, 6:WEP,
       | 7:WPA-PSK, 8: WPA-Auto-Personal
       | [-w, --ieee-80211w]: MFP (optional: needs security type to be specified)
       | : 0:Disable, 1:Optional, 2:Required.
       | [-m, --bssid]: MAC address of the AP (BSSID).
       | [-t, --timeout]: Duration after which connection attempt needs to fail.
       | [-h, --help]: Print out the help for the connect command.
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
       |
       | disable - Disable Access Point mode
       | (Note that the Access Point mode is presently not supported.)
       |
       | stations  : List stations connected to the AP
       |
       | disconnect - Disconnect a station from the AP
       | <MAC address of the station>
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
       | quick_setup   : Start a TWT flow with defaults:
       |  <twt_wake_interval: 1-262144us> <twt_interval: 1us-2^31us>.
       |
       | setup         : Start a TWT flow:
       |  <negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>
       |  <setup_cmd: 0: Request, 1: Suggest, 2: Demand>
       |  <dialog_token: 1-255> <flow_id: 0-7> <responder: 0/1> <trigger:
       |  0/1> <implicit:0/1> <announce: 0/1> <twt_wake_interval:
       |  1-262144us> <twt_interval: 1us-2^31us>.
       |
       | teardown      : Teardown a TWT flow:
       |  <negotiation_type, 0: Individual, 1: Broadcast, 2: Wake TBTT>
       |  <setup_cmd: 0: Request, 1: Suggest, 2: Demand>
       |  <dialog_token: 1-255> <flow_id: 0-7>.
       |
       | teardown_all  : Teardown all TWT flows.
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
   * - mode
     - | This command may be used to set the Wi-Fi device into a specific mode of operation
       | parameters:
       | [-i : --if-index <idx>] : Interface index.
       | [-s : --sta] : Station mode.
       | [-m : --monitor] : Monitor mode.
       | [-p : --promiscuous] : Promiscuous mode.
       | [-t : --tx-injection] : TX-Injection mode.
       | [-a : --ap] : AP mode.
       | [-k : --softap] : Softap mode.
       | [-h : --help] : Help.
       | [-g : --get] : Get current mode for a specific interface index
       | Usage: Get operation example for interface index 1
       | wifi mode -g -i1
       | Set operation example for interface index 1 - set station+promiscuous
       | wifi mode -i1 -sp
   * - packet_filter
     - | This command is used to set packet filter setting when
       | monitor, TX-Injection and promiscuous mode is enabled
       | The different packet filter modes are control,
       | management, data and enable all filters
       | [-i, --if-index <idx>] : Interface index
       | [-a, --all] : Enable all packet filter modes
       | [-m, --mgmt] : Enable management packets to be allowed up
       | the stack
       | [-c, --ctrl] : Enable control packets to be allowed up
       | the stack
       | [-d, --data] : Enable Data packets to be allowed up the
       | stack
       | [-g, --get] : Get current filter settings for a specific
       | interface index
       | [-b, --capture-len <len>] : Capture length buffer size
       | for each packet to be captured
       | [-h, --help] : Help
       | Usage: Get operation example for interface index 1
       | wifi packet_filter -g -i1
       | Set operation example for interface index 1 - set
       | data+management frame filter
       | wifi packet_filter -i1 -md
   * - channel
     - | This command is used to set the channel when monitor or TX-Injection mode is enabled
       | Currently 20 MHz is only supported and no BW parameter is provided
       | parameters:
       | [-i : --if-index <idx>] : Interface index.
       | [-c : --channel] : Set a specific channel number to the lower layer.
       | [-g : --get] : Get current set channel number from the lower layer.
       | [-h : --help] : Help.
       | Usage: Get operation example for interface index 1
       | wifi channel -i1 -g
       | Set operation example for interface index 1 (setting channel 5)
       | wifi -i1 -c5

``wifi_cred`` is an extension to the Wi-Fi command line.
It adds the following subcommands to interact with the :ref:`lib_wifi_credentials` library:

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

``raw_tx`` is an extension to the Wi-Fi command line.
It adds the following subcommands to configure and send raw TX packets:

.. list-table:: raw TX shell subcommands
   :header-rows: 1

   * - Subcommands
     - Description
     - Valid values
   * - mode
     - | Enable or Disable TX injection mode
       | [-h, --help]: Print out the help for the mode command
     - | Valid values:
       | 1 - Enable
       | 0 - Disable
   * - configure
     - | Configure the raw TX packet header with the following parameters:
       | [-f, --rate-flags]: Rate flag value
       | [-d, --data-rate]: Data rate value
       | [-q, --queue-number]: Queue number
       | [-h, --help]: Print out the help for the configure command
     - | Valid Rate flag values:
       | 0 - Legacy
       | 1 - HT mode
       | 2 - VHT mode
       | 3 - HE (SU) mode
       | 4 - HE (ERSU) mode
       |
       | Valid Data rate values:
       | Legacy: 1, 2, 55, 11, 6, 9, 12, 18, 24, 36, 48, 54
       | Non-Legacy: MCS index need to be used (0 - 7)
       |
       | Valid Queue numbers:
       | 0 - Background
       | 1 - Best effort
       | 2 - Video
       | 3 - Voice
       | 4 - Beacon
   * - send
     - | Send raw TX packets
       | parameters:
       | [-m, --mode]: Mode of transmission (either continuous or fixed)
       | [-n, --number-of-pkts]: Number of packets to be transmitted
       | [-t, --inter-frame-delay]: Delay between frames or packets in milliseconds
       | [-h, --help]: Print out the help for the send command
     - | N/A

For more information, see :ref:`ug_nrf70_developing_raw_ieee_80211_packet_transmission`.

``promiscuous_set`` is an extension to the Wi-Fi command line.
It adds the following subcommand to configure Promiscuous mode:

.. list-table:: Promiscuous mode shell subcommand
   :header-rows: 1

   * - Subcommand
     - Description
     - Valid values
   * - mode
     - | Enable or Disable Promiscuous mode
       | [-h, --help]: Print out the help for the mode command
     - | Valid values:
       | 1 - Enable
       | 0 - Disable

For more information, see :ref:`ug_nrf70_developing_promiscuous_packet_reception`.

Testing STA mode
================

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Scan for the Wi-Fi networks in range using the following command:

   .. code-block:: console

      wifi scan

   The output should be similar to the following:

   .. code-block:: console

      Scan requested

      Num  | SSID                             (len) | Chan (Band)    | RSSI | Security        | BSSID
      1    | xyza                             4     | 1    (2.4GHz)  | -27  | WPA2-PSK        | xx:xx:xx:xx:xx:xx
      2    | abcd                             4     | 149  (5GHz  )  | -28  | WPA2-PSK        | yy:yy:yy:yy:yy:yy



#. Connect to your preferred network using the following command:

   .. code-block:: console

      wifi connect -s <SSID> -k <key_management> -p <passphrase>

   ``<SSID>`` is the SSID of the network you want to connect to, ``<passphrase>`` is its passphrase, and the ``<key_management>`` is the security type used by the network.

#. Check the connection status after a while, using the following command:

   .. code-block:: console

      wifi status

   If the connection is established, you should see an output similar to the following:

   .. code-block:: console

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

#. Initiate a ping and verify data connectivity using the following commands:

   .. code-block:: console

      net dns <hostname>
      net ping <resolved hostname>

   See the following example:

   .. code-block:: console

      net dns google.com
       Query for 'google.com' sent.
       dns: 142.250.74.46
       dns: All results received

      net ping 10 142.250.74.46
       PING 142.250.74.46
       28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=0 ttl=113 time=191 ms
       28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=1 ttl=113 time=190 ms
       28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=2 ttl=113 time=190 ms

Testing SAP mode
================

To test the SAP mode, the sample must be built using the configuration overlay :file:`overlay-sap.conf` file.

|test_sample|

#. |connect_kit|
#. |connect_terminal|

#. Set the appropriate regulatory domain using the following command:

   .. code-block:: console

      wifi reg_domain <ISO/IEC 3166-1 alpha2>

   For example, to set the regulatory domain to IN, use the following command:

   .. code-block:: console

      wifi reg_domain IN
#. Set an IP address for the SAP interface using the following command:

   .. code-block:: console

      net ipv4 add 1 192.168.1.1 255.255.255.0

#. Enable the Access Point mode using the following command:

   .. code-block:: console

      wifi ap enable -s <SSID> -c <channel> -k <key_management> -p <psk>

   ``<SSID>`` is the SSID of the network you want to connect to, ``<psk>`` is its passphrase, and the ``<key_management>`` is the security type used by the network.

#. Check the SAP status after a while, using the following command:

   .. code-block:: console

        wifi status

   If the SAP is established, you should see an output similar to the following:

   .. code-block:: console

        Status: successful
        ==================
        State: COMPLETED
        Interface Mode: ACCESS POINT
        Link Mode: UNKNOWN
        SSID: testing
        BSSID: F4:CE:36:00:22:C6
        Band: 2.4GHz
        Channel: 1
        Security: OPEN
        MFP: Disable
        Beacon Interval: 0
        DTIM: 2
        TWT: Not supported

#. Connect a station to the SAP using a static IP address and verify the connection using the following command:

   .. code-block:: console

        wifi ap stations

   If the station is connected, you should see an output similar to the following:

   .. code-block:: console

        AP stations:
        ============
        Station 1:
        ==========
        MAC: 62:26:54:D9:1C:6E
        Link mode: WIFI 4 (802.11n/HT)
        TWT: Not supported

#. Verify connectivity by pinging the Station from the SAP using the following command:

   .. code-block:: console

      net ping <station IP address>

   See the following example:

   .. code-block:: console

      net ping 192.168.1.88
        PING 192.168.1.88
        28 bytes from 192.168.1.88 to 192.168.1.1: icmp_seq=1 ttl=64 time=5 ms
        28 bytes from 192.168.1.88 to 192.168.1.1: icmp_seq=2 ttl=64 time=5 ms
        28 bytes from 192.168.1.88 to 192.168.1.1: icmp_seq=3 ttl=64 time=5 ms

#. Disable the Access Point mode using the following command:

   .. code-block:: console

      wifi ap disable

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`

This sample also uses modules found in the following locations in the |NCS| folder structure:

* :file:`modules/lib/hostap`
* :file:`modules/mbedtls`
