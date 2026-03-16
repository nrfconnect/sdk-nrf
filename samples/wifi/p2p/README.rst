.. _wifi_p2p_sample:

Wi-Fi: P2P
##########

.. contents::
   :local:
   :depth: 2

The P2P sample demonstrates how to establish a Wi-Fi® Peer-to-Peer (P2P) connection between devices using the Wi-Fi Direct® functionality.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates Wi-Fi P2P functionality using the following two operating modes:

* Client (CLI) mode - The device discovers a peer and initiates a P2P connection.
* Group Owner (GO) mode - The device acts as the P2P Group Owner and manages the P2P group.

Wi-Fi P2P enables direct communication between devices without requiring a traditional access point.

For more information, see :ref:`Wi-Fi Direct (P2P) mode <ug_wifi_direct>`.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/p2p/Kconfig`):

.. options-from-kconfig::
   :show-type:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/p2p`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp

.. include:: /includes/wifi_refer_sample_yaml_file.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      *** Booting nRF Connect SDK v3.2.99-aa13127d2ec2 ***
      *** Using Zephyr OS v4.3.99-1d6a9d7577c0 ***
      [00:00:00.328,369] <inf> net_config: Initializing network
      [00:00:00.328,369] <inf> net_config: Waiting interface 1 (0x20001318) to be up...
      [00:00:00.328,582] <inf> net_config: IPv4 address: 192.168.1.99
      [00:00:00.328,613] <inf> net_config: Running dhcpv4 client...
      [00:00:00.328,887] <inf> p2p_cli: Starting nrf7002dk with CPU frequency: 64 MHz
      [00:00:00.329,589] <inf> wifi_supplicant: wpa_supplicant initialized
      [00:00:01.331,634] <inf> p2p_cli: P2P find started
      [00:00:05.950,012] <inf> wpa_supp: P2P-DEVICE-FOUND 2a:94:01:b5:19:ee p2p_dev_addr=28:94:01:b5:19:ee pri_dev_type=10-0050F204-5 name='P2P_DEVICE' config_methods=0x88 dev_capab=0x25 gro1
      [00:00:05.951,568] <inf> wpa_supp: P2P-DEVICE-FOUND d2:39:fa:43:23:c1 p2p_dev_addr=d2:39:fa:43:23:c1 pri_dev_type=10-0050F204-5 name='Galaxy S22' config_methods=0x188 dev_capab=0x25 gr1
      [00:00:05.952,941] <inf> p2p_cli: Peer found, initializing connection
      [00:00:05.956,359] <inf> wpa_supp: P2P-FIND-STOPPED
      [00:00:05.958,068] <inf> p2p_cli: PIN: 74834488
      [00:00:07.994,812] <err> wpa_supp: Failed to allocated for event: 19
      [00:00:15.589,416] <inf> wpa_supp: P2P-GO-NEG-SUCCESS role=client freq=2462 ht40=0 peer_dev=d2:39:fa:43:23:c1 peer_iface=d2:39:fa:43:a3:c1 wps_method=Display
      [00:00:15.589,996] <inf> wpa_supp: wlan0: WPS-PIN-ACTIVE
      [00:00:16.631,103] <inf> wpa_supp: wlan0: SME: Trying to authenticate with d2:39:fa:43:a3:c1 (SSID='DIRECT-V2-Galaxy S22' freq=2462 MHz)
      [00:00:17.001,617] <inf> wpa_supp: wlan0: Trying to associate with d2:39:fa:43:a3:c1 (SSID='DIRECT-V2-Galaxy S22' freq=2462 MHz)
      [00:00:17.122,283] <wrn> net_if: iface 1 pkt 0x2002bb0c send failure status -1
      [00:00:17.126,800] <inf> wpa_supp: wlan0: Associated with d2:39:fa:43:a3:c1
      [00:00:17.127,929] <inf> wpa_supp: wlan0: CTRL-EVENT-SUBNET-STATUS-UPDATE status=0
      [00:00:17.136,962] <inf> wpa_supp: wlan0: CTRL-EVENT-EAP-STARTED EAP authentication started
      [00:00:17.148,529] <inf> wpa_supp: wlan0: CTRL-EVENT-EAP-PROPOSED-METHOD vendor=14122 method=1
      [00:00:17.149,169] <inf> wpa_supp: wlan0: CTRL-EVENT-EAP-METHOD EAP vendor 14122 method 1 (WSC) selected
      [00:00:17.149,597] <inf> wpa_supp: wlan0: CTRL-EVENT-EAP-FAILURE EAP authentication failed
      [00:00:17.152,038] <inf> wpa_supp: wlan0: Added BSSID d2:39:fa:43:a3:c1 into ignore list, ignoring for 10 seconds
      [00:00:17.153,289] <inf> wpa_supp: wlan0: CTRL-EVENT-DISCONNECTED bssid=d2:39:fa:43:a3:c1 reason=3 locally_generated=1
      [00:00:17.153,472] <inf> p2p_cli: Connected
      [00:00:17.153,869] <inf> wpa_supp: wlan0: BSSID d2:39:fa:43:a3:c1 ignore list count incremented to 2, ignoring for 10 seconds
      [00:00:17.221,466] <err> wpa_supp: wpa_drv_zep_event_proc_scan_res: Failed to alloc scan results(525 bytes)
      [00:00:17.530,761] <inf> wpa_supp: wlan0: Removed BSSID d2:39:fa:43:a3:c1 from ignore list (clear)
      [00:00:17.532,318] <inf> wpa_supp: wlan0: SME: Trying to authenticate with d2:39:fa:43:a3:c1 (SSID='DIRECT-V2-Galaxy S22' freq=2462 MHz)
      [00:00:17.546,752] <inf> wpa_supp: wlan0: Trying to associate with d2:39:fa:43:a3:c1 (SSID='DIRECT-V2-Galaxy S22' freq=2462 MHz)
      [00:00:17.615,051] <inf> wpa_supp: wlan0: Associated with d2:39:fa:43:a3:c1
