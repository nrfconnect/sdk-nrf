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

The sample demonstrates Wi-Fi scan operations in the 2.4 GHz and 5 GHz bands.
The default scan type is active.
However, you can configure the scan type to be either active or passive.
Also, you can perform scan operation based on

* Frequency band(s)
* Specific SSID(s)
* Specific channel(s)

Using this sample, the development kit can scan for available access points in :abbr:`STA (Station)` mode.

Configuration
*************

|config|

Select the scan type to be used by the sample.

.. note::
        You can also use ``menuconfig`` to enable the ``Scan type`` option.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Configuration options
=====================

The following sample-specific Kconfig option is used in this sample (located in :file:`samples/wifi/scan/Kconfig`) :

.. _CONFIG_WIFI_SCAN_TYPE_SELECT:

CONFIG_WIFI_SCAN_TYPE_SELECT
   This option sets the scan type to be used by the sample.

.. options-from-kconfig::
   :show-type:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/scan`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following are examples of the CLI commands:

* Build to fetch only Device scan results

  .. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

* Build to fetch only Raw scan results

  .. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_WIFI_MGMT_RAW_SCAN_RESULTS=y -DCONFIG_WIFI_MGMT_RAW_SCAN_RESULTS_ONLY=y

* Build to fetch both Raw and Device scan results

  .. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_WIFI_MGMT_RAW_SCAN_RESULTS=y

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

* Device scan only results:

  .. code-block:: console

      Scan requested
      Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
      1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
      2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
      3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
      Scan request done

* Raw scan only results:

  .. code-block:: console

      Scan requested
      Num  | len   | Frequency | RSSI | RAW_DATA(32 bytes)
      1    | 299   | 2412 | -44  | 50 00 3A 01 F4 CE 36 00 10 CE D4 BD 4F E1 F5 33 D4 BD 4F E1 F5 33 A0 7F 0B 05 75 7F D8 00 00 00
      2    | 430   | 2412 | -37  | 50 00 3A 01 F4 CE 36 00 10 CE 3C 7C 3F DA DF 38 3C 7C 3F DA DF 38 90 11 69 36 EC 8A 02 00 00 00
      3    | 284   | 2412 | -45  | 50 00 3A 01 F4 CE 36 00 10 CE D4 BD 4F 21 F5 38 D4 BD 4F 21 F5 38 20 CF 7B 20 75 7F D8 00 00 00
      4    | 299   | 2412 | -43  | 50 00 3A 01 F4 CE 36 00 10 CE D4 BD 4F E1 F5 33 D4 BD 4F E1 F5 33 B0 7F EC 2B 75 7F D8 00 00 00
      5    | 409   | 2412 | -38  | 50 00 3A 01 F4 CE 36 00 10 CE A0 36 BC 56 41 E0 A0 36 BC 56 41 E0 60 E2 C7 A5 0F 90 0D 00 00 00
      Scan request done

* Raw scan and Device scan results:

  .. code-block:: console

      Scan requested
      Num  | len   | Frequency | RSSI | RAW_DATA(32 bytes)
      1    | 299   | 2412 | -44  | 50 00 3A 01 F4 CE 36 00 10 CE D4 BD 4F E1 F5 33 D4 BD 4F E1 F5 33 A0 7F 0B 05 75 7F D8 00 00 00
      2    | 430   | 2412 | -37  | 50 00 3A 01 F4 CE 36 00 10 CE 3C 7C 3F DA DF 38 3C 7C 3F DA DF 38 90 11 69 36 EC 8A 02 00 00 00
      3    | 284   | 2412 | -45  | 50 00 3A 01 F4 CE 36 00 10 CE D4 BD 4F 21 F5 38 D4 BD 4F 21 F5 38 20 CF 7B 20 75 7F D8 00 00 00
      4    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
      5    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
      6    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
      Scan request done

* Default (Active) scan results:

  .. code-block:: console

      Scan requested
      Num  | SSID                           (len) | Chan | RSSI | Security | BSSID
      1    |                                  0   | 11   | -39  | Open     | C2:A5:11:A2:B1:E2
      2    | abcdefg                          7   | 11   | -39  | Open     | BC:A5:11:A2:B1:E2
      3    | hijklmno                         8   | 48   | -43  | Open     | 00:22:CF:E6:AE:99
      Scan request done

* Passive scan results:

  .. code-block:: console

      Scan requested
      Num  | SSID                           (len) | Chan | RSSI | Security | BSSID
      1    |                                  0   | 11   | -39  | Open     | C2:A5:11:A2:B1:E2
      2    | abcdefg                          7   | 11   | -39  | Open     | BC:A5:11:A2:B1:E2
      3    | hijklmno                         8   | 48   | -43  | Open     | 00:22:CF:E6:AE:99
      Scan request done

* 2.4 GHz scan only results:

  .. code-block:: console

      Scan requested
      Num  | SSID                           (len) | Chan | RSSI | Security | BSSID
      1    | abcdefg                          7   | 11   | -39  | Open     | BC:A5:11:A2:B1:E2
      Scan request done

* 5 GHz scan only results:

  .. code-block:: console

      Scan requested
      Num  | SSID                           (len) | Chan | RSSI | Security | BSSID
      1    | pqrst                             5  | 48   | -43  | Open     | 00:22:CF:E6:AE:99
      Scan request done

* SSID based scan results:

  .. code-block:: console

      Scan requested
      Num  | SSID                           (len) | Chan | RSSI | Security | BSSID
      1    | pqrst                             5  | 48   | -43  | Open     | 00:22:CF:E6:AE:99
      Scan request done

* Channel based scan results:

  .. code-block:: console

      Scan requested
      Num  | SSID                           (len) | Chan | RSSI | Security | BSSID
      1    | abcdefg                          7   | 11   | -39  | Open     | BC:A5:11:A2:B1:E2
      1    | pqrst                            5   | 48   | -43  | Open     | 00:22:CF:E6:AE:99
      Scan request done
