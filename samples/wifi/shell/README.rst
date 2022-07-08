.. _wifi_sample:

Wi-Fi sample
############

This sample allows testing Nordic Wi-Fi driver for various boards by enabling the Wi-Fi supplicant module that provides the ``scan``, ``connect``, and ``disconnect`` commands.
It also enables the :ref:`zephyr:net_shell` module to verify ``net_if`` settings.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf7002dk_nrf5340_cpuapp

Building and Running
********************

.. |sample path| replace:: :file:`samples/wifi/shell`

.. include:: /includes/build_and_run_ns.txt

Currently, the following configurations are supported:

* 7002 DK + QSPI
* 7002 EK + SPIM

You can build for the nRF7002 DK by using the following command::

   west build -b nrf7002dk_nrf5340_cpuapp

You can build for the nRF7002 EK by using the following command::

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002_ek

Sample console interaction
==========================

The following an example of a console interaction:

.. code-block:: console

   uart:~$ wpa_cli scan
   Scan requested
   Num  | SSID                             (len) | Chan | RSSI | Sec
   1    | dlink-7D64                       10    | 1    | 0    | WPA/WPA2
   2    | abcd_527_24                      11    | 4    | 0    | Open
   3    | ASUS_RTAX88U11g                  15    | 3    | 0    | WPA/WPA2
   4    | TP-Link_6BA8                     12    | 9    | 0    | WPA/WPA2
   uart:~$ wpa_cli connect NORDIC-GUEST <passphrase>
   wlan0: WPA: Key negotiation completed with f0:1d:2d:73:c4:cf [PTK=CCMP GTK=CCMP]
   wlan0: Cancelling authentication timeout
   wlan0: State: GROUP_HANDSHAKE -> COMPLETED
   wlan0: Radio work 'sme-connect'@0x20044688 done in 1.506379 seconds
   wlan0: radio_work_free('sme-connect'@0x20044688): num_active_works --> 0
   wlan0: CTRL-EVENT-CONNECTED - Connection to f0:1d:2d:73:c4:cf completed [id=0 id_str=]

   .. wait for 15/20 seconds for DHCP to get resolved

   <inf> net_dhcpv4: Received: 192.168.1.101[00:01:01.078,735]
   <err> net_mgmt: Event info length 48 > max size 43[00:01:01.078,826]
   <inf> net_config: IPv4 address: 192.168.1.101[00:01:01.078,826]
   <inf> net_config: Lease time: 86400 seconds[00:01:01.078,857]
   <inf> net_config: Subnet: 255.255.255.0[00:01:01.078,887]
   <inf> net_config: Router: 192.168.1.1

   uart:~$ net dns google.com
   Query for 'google.com' sent.
   dns: 142.250.74.46
   dns: All results received

   uart:~$ net ping -c 10 142.250.74.46
   PING 142.250.74.46
   28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=0 ttl=113 time=191 ms
   28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=1 ttl=113 time=190 ms
   28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=2 ttl=113 time=190 ms
   28 bytes from 142.250.74.46 to 192.168.50.199: icmp_seq=3 ttl=113 time=192 ms

Dependencies
============

This sample uses the following Zephyr module:

* :ref:`zephyr:net_shell`
