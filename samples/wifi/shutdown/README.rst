.. _wifi_shutdown_sample:

Wi-Fi: Shutdown
###############

.. contents::
   :local:
   :depth: 2

The Shutdown sample demonstrates how to put the Nordic Semiconductor's Wi-Fi® chipset in shutdown mode.
This also demonstrates how to achieve the lowest possible power consumption in the nRF5340 SoC when Wi-Fi is enabled but not in use.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample can demonstrate Wi-Fi shutdown and achieve the lowest possible power consumption in the nRF5340 SoC.
The sample:

1. Initializes the Wi-Fi driver.
#. Scans for available Wi-Fi networks to verify that the Wi-Fi driver is operational.
#. Shuts down the Wi-Fi driver.
#. Puts the nRF5340 SoC in the lowest possible power consumption mode.

User Interface
**************

Button 1:
   Wakes up the nRF5340 SoC and initializes the Wi-Fi chipset.
   The sample then scans for available Wi-Fi networks to verify that the Wi-Fi driver is operational.

Button 2:
   Shuts down the Wi-Fi driver.
   The nRF5340 SoC is put into the lowest possible power consumption mode.


Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/shutdown`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command to demonstrate Wi-Fi shutdown:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp


Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      *** Booting Zephyr OS build v3.3.99-ncs1-26-ge405279d2134 ***
      [00:00:00.440,460] <inf> wifi_nrf: Firmware (v1.2.8.1) booted successfully

      [00:00:00.638,397] <inf> scan: Starting nrf7002dk_nrf5340_cpuapp with CPU frequency: 64 MHz

      [00:00:00.642,608] <inf> scan: Scan requested

      Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
      1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
      2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
      3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
      [00:00:05.445,739] <inf> scan: Scan request done

      [00:00:05.452,423] <inf> scan: Interface down

#. Press **Button 1** to wake up the nRF5340 SoC, initialize the Wi-Fi chipset, and scan for available Wi-Fi networks:

   The sample shows the following output:

   .. code-block:: console

      [00:00:29.141,357] <inf> wifi_nrf: Firmware (v1.2.8.1) booted successfully

      [00:00:29.269,165] <inf> scan: Interface up
      [00:00:29.272,521] <inf> scan: Scan requested

      Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
      1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
      2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
      3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
      [00:00:34.092,285] <inf> scan: Scan request done

#. Press **Button 2** to shut down the Wi-Fi driver and put the nRF5340 SoC in lowest possible power consumption mode:

   The sample shows the following output:

   .. code-block:: console

      [00:00:48.313,354] <inf> scan : Interface down
