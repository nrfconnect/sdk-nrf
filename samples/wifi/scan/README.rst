.. _wifi_scan_sample:

Wi-Fi: Scan
############

.. contents::
   :local:
   :depth: 2

This sample allows the Nordic Wi-Fi chipset to scan for the Access points.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample can perform Wi-Fi "Scan" operation over both 2.4GHz and 5GHz

Using this sample, the development kit can scan for available Access Points in :abbr:`STA (Station)` mode.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/scan`

.. include:: /includes/build_and_run_ns.txt

Currently, the following board(s) are supported:

* nRF7002 DK


To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command::

   west build -b nrf7002dk_nrf5340_cpuapp

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
   The output should be similar to the following::

      <inf> scan: Scan requested
      Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
      1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
      2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
      3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
      <inf> scan: Scan request done
