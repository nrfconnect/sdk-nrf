.. _wifi_scan_sample:

Wi-Fi: Scan
############

.. contents::
   :local:
   :depth: 2

The Scan sample demonstrates how to use the Nordic Semiconductor's Wi-Fi® chipset to scan for the access points without using the wpa_supplicant.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

This sample can perform Wi-Fi scan operations in the 2.4GHz and 5GHz bands.

Using this sample, the development kit can scan for available access points in :abbr:`STA (Station)` mode.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/scan`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      <inf> scan: Scan requested
      Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
      1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
      2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
      3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
      <inf> scan: Scan request done
