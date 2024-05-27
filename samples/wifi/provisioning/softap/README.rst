.. _softap_wifi_provision_sample:

Wi-Fi: SoftAP based provision
#############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`lib_softap_wifi_provision` library to provision credentials to a Nordic Semiconductor Wi-Fi® device.

.. |wifi| replace:: Wi-Fi

.. include:: /includes/net_connection_manager.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

After boot, the device starts advertising in SoftAP mode with the SSID name set by the :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SSID` option (default is *nrf-wifiprov*), allowing clients (STA) to connect to it and provide Wi-Fi credentials.
See :ref:`SoftAP Wi-Fi Provision library HTTP resources <softap_wifi_provision_http_resources>` for more information on how to configure the SoftAP network and the HTTP resources that are available.

User interface
**************

During provisioning, the device's LEDs indicate the current state of the provisioning process.

LEDs
====

LED 1:
   Turns on, then the device is advertising the SoftAP network.

LED 2:
   Turns on when the device is connected to the provisioned Wi-Fi network.

Configuration
*************

|config|

Configuration files
===================

The sample includes the following pre-configured configuration file for the development kits that are supported:

* :file:`prj.conf` - Configuration file for all build targets.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/provisioning/softap`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

To test the sample, you can use the `nRF Wi-Fi Provisioner mobile app for Android`_ or `nRF Wi-Fi Provisioner mobile app for iOS`_ to provision the device with Wi-Fi credentials.

The following are the source code for these:

* `Source code for nRF Wi-Fi Provisioner mobile app for Android`_
* `Source code for nRF Wi-Fi Provisioner mobile app for iOS`_

Alternatively, you can use the provided Python script by completing the following steps:

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      *** Using Zephyr OS v3.6.99-25fbeabe9004 ***
      [00:00:00.662,811] <inf> softap_wifi_provision_sample: SoftAP Wi-Fi provision sample started
      [00:00:00.673,187] <dbg> softap_wifi_provision: init_entry: Registering self-signed server certificate
      [00:00:08.612,731] <dbg> softap_wifi_provision: certificate_register: Self-signed server certificate provisioned
      [00:00:08.652,038] <inf> softap_wifi_provision_sample: Network interface brought up
      [00:00:08.660,858] <dbg> softap_wifi_provision: process_tcp4: Waiting for IPv4 HTTP connections on port 443
      [00:00:08.671,020] <dbg> softap_wifi_provision: wifi_scan: Scanning for Wi-Fi networks...
      [00:00:13.301,605] <dbg> softap_wifi_provision: net_mgmt_wifi_event_handler: NET_EVENT_WIFI_SCAN_DONE
      [00:00:13.311,401] <dbg> softap_wifi_provision: unprovisioned_exit: Scanning for Wi-Fi networks completed, preparing protobuf payload
      [00:00:13.324,523] <dbg> softap_wifi_provision: unprovisioned_exit: Protobuf payload prepared, scan results encoded, size: 212
      [00:00:13.336,212] <dbg> softap_wifi_provision: provisioning_entry: Enabling AP mode to allow client to connect and provide Wi-Fi credentials.
      [00:00:13.349,273] <dbg> softap_wifi_provision: provisioning_entry: Waiting for Wi-Fi credentials...
      [00:00:14.034,057] <dbg> softap_wifi_provision: net_mgmt_wifi_event_handler: NET_EVENT_WIFI_AP_ENABLE_RESULT
      [00:00:14.046,051] <inf> softap_wifi_provision_sample: Provisioning started
      [00:00:14.054,565] <dbg> softap_wifi_provision: dhcp_server_start: IPv4 address added to interface
      [00:00:14.063,964] <dbg> softap_wifi_provision: dhcp_server_start: IPv4 netmask set
      [00:00:14.072,296] <dbg> softap_wifi_provision: dhcp_server_start: DHCPv4 server started

#. Connect to the softAP Wi-Fi network, the default SSID is *nrf-wifiprov*, and observe the following output:

   .. code-block:: console

      [00:01:18.940,307] <dbg> softap_wifi_provision: net_mgmt_wifi_event_handler: NET_EVENT_WIFI_AP_STA_CONNECTED
      [00:01:18.950,469] <dbg> softap_wifi_provision: print_mac: Client STA connected, MAC: 04:00:91:9E:31:EA
      [00:01:18.960,174] <inf> softap_wifi_provision_sample: Client connected

#. Generate python structures from the protobuf schema by running the following command:

   .. code-block:: console

      cd <sample-dir>/scripts
      protoc --proto_path=<nrf-sdk-dir>/subsys/net/lib/softap_wifi_provision/proto --python_out=. common.proto

#. Start provisioning by calling the following command:

   .. code-block:: console

      python3 provision.py --certificate ../certs/server_certificate.pem

   The script performs HTTP calls to the device, fetches the Wi-Fi networks that are available to it, parses the response using the supported protobuf schema, and displays the networks to you.
   It prompts you to select a network and enter the password for it.

   .. code-block:: console

      python3 provision.py --certificate ../certs/server_certificate.pem
      0: SSID: cia-linksys, BSSID: d8ec5e8f6619, RSSI: -49, Band: BAND_5_GHZ, Channel: 112, Auth: WPA_WPA2_PSK
      1: SSID: cia-asusgold, BSSID: c87f54de23b8, RSSI: -49, Band: BAND_2_4_GHZ, Channel: 9, Auth: WPA_WPA2_PSK
      2: SSID: cia-linksys, BSSID: d8ec5e8f6618, RSSI: -50, Band: BAND_5_GHZ, Channel: 44, Auth: WPA_WPA2_PSK
      3: SSID: NORDIC-GUEST, BSSID: 2436da11c0af, RSSI: -52, Band: BAND_5_GHZ, Channel: 60, Auth: WPA_WPA2_PSK
      4: SSID: NORDIC-INTERNAL, BSSID: 2436da11c0ae, RSSI: -53, Band: BAND_5_GHZ, Channel: 60, Auth: WPA_WPA2_PSK
      Select the network (number): 1
      Enter the passphrase for the network: xxxxxxxxxxxxxxx
      Length of serialized data:  47
      Configuration successful!

#. Observe the following output from the device and see that it connects to the selected network.

   .. code-block:: console

      [00:00:21.947,784] <inf> softap_wifi_provision_sample: Client connected
      [00:00:26.947,174] <dbg> softap_wifi_provision: process_tcp: [15] Connection from 192.168.0.2 accepted
      [00:00:27.018,859] <dbg> softap_wifi_provision: on_message_begin: on_message_begin, method: 1
      [00:00:27.027,770] <dbg> softap_wifi_provision: on_url: on_url, method: 1
      [00:00:27.034,881] <dbg> softap_wifi_provision: on_url: > /prov/networks
      [00:00:27.042,053] <dbg> softap_wifi_provision: on_headers_complete: on_headers_complete, method: GET
      [00:00:27.051,574] <dbg> softap_wifi_provision: on_message_complete: on_message_complete, method: 1
      [00:00:27.062,408] <dbg> softap_wifi_provision: process_tcp: Closing listening socket: 15
      [00:00:37.169,372] <dbg> softap_wifi_provision: process_tcp: [15] Connection from 192.168.0.2 accepted
      [00:00:37.243,316] <dbg> softap_wifi_provision: on_message_begin: on_message_begin, method: 3
      [00:00:37.252,197] <dbg> softap_wifi_provision: on_url: on_url, method: 3
      [00:00:37.259,338] <dbg> softap_wifi_provision: on_url: > /prov/configure
      [00:00:37.266,632] <dbg> softap_wifi_provision: on_headers_complete: on_headers_complete, method: POST
      [00:00:37.281,585] <dbg> softap_wifi_provision: on_body: on_body: 3
      [00:00:37.288,177] <dbg> softap_wifi_provision: on_body: on_body length: 38
      [00:00:37.295,532] <dbg> softap_wifi_provision: on_body: on_body: 3
      [00:00:37.302,124] <dbg> softap_wifi_provision: on_body: on_body length: 47
      [00:00:37.309,448] <dbg> softap_wifi_provision: on_message_complete: on_message_complete, method: 3
      [00:00:37.318,847] <dbg> softap_wifi_provision: parse_and_store_credentials: HTTP body
                                                      0a 1c 0a 0c 63 69 61 2d  61 73 75 73 67 6f 6c 64 |....cia- asusgold
                                                      12 06 c8 7f 54 de 23 b8  18 01 20 01 28 04 12 0f |....T.#. .. .(...
                                                      74 68 69 6e 67 79 77 6f  72 6c 64 32 30 32 34    |xxxxxxxx xxxxxxx
      [00:00:37.357,513] <dbg> softap_wifi_provision: parse_and_store_credentials: Received Wi-Fi credentials: ssid: cia-asusgold, bssid: C8:7F:54:DE:23:B8, passphrase: xxxxxx, sectype: 4, channel: 1, band: 1
      [00:00:37.468,536] <inf> softap_wifi_provision_sample: Wi-Fi credentials received
      [00:00:37.477,172] <dbg> softap_wifi_provision: process_tcp: Closing listening socket: 15
      [00:00:37.490,356] <dbg> softap_wifi_provision: process_tcp4: Credentials stored, stop processing of incoming requests
      [00:00:37.501,342] <dbg> softap_wifi_provision: process_tcp4: Leaving server socket open to keep mDNS SD functioning
      [00:00:37.512,695] <dbg> softap_wifi_provision: provisioning_exit: Credentials received, cleaning up...
      [00:00:38.534,362] <dbg> softap_wifi_provision: net_mgmt_wifi_event_handler: NET_EVENT_WIFI_AP_STA_DISCONNECTED
      [00:00:38.544,769] <dbg> softap_wifi_provision: print_mac: Client STA disconnected, MAC: 00:00:91:9E:31:EA
      [00:00:38.554,718] <inf> softap_wifi_provision_sample: Client disconnected
      [00:00:38.718,566] <dbg> softap_wifi_provision: net_mgmt_wifi_event_handler: NET_EVENT_WIFI_AP_DISABLE_RESULT
      [00:00:38.728,881] <inf> softap_wifi_provision_sample: Provisioning completed
      [00:00:40.377,868] <inf> wifi_mgmt_ext: Connection requested
      [00:00:40.389,312] <inf> softap_wifi_provision_sample: PSM disabled
      [00:00:40.819,732] <inf> softap_wifi_provision_sample: Network connected
      [00:02:40.399,627] <inf> softap_wifi_provision_sample: PSM enabled

Troubleshooting
===============

For issues related to the library and |NCS| in general, refer to :ref:`known_issues`.

If the hostname is changed by updating the :kconfig:option:`CONFIG_NET_HOSTNAME` Kconfig option, the server certificate must be re-generated with the Subject Alternate Name (SAN) field set to the new hostname.
This is to ensure that Server Name Indication (SNI) succeeds during the TLS handshake.

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`lib_softap_wifi_provision`
* :ref:`net_mgmt_interface`
* :ref:`Connection Manager <zephyr:conn_mgr_overview>`
