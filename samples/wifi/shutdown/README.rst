.. _wifi_shutdown_sample:

Wi-Fi: Shutdown
###############

.. contents::
   :local:
   :depth: 2

The Shutdown sample demonstrates how to put the Nordic Semiconductor's Wi-Fi® chipset in the Shutdown state, where the device is completely powered off.
For more information, see the `nRF70 Series power states`_ page.

This also demonstrates how to achieve the lowest possible power consumption in the Host SoC (nRF53, nRF52 or nRF91 Series) when Wi-Fi is enabled but not being used.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample can demonstrate Wi-Fi shutdown and achieve the lowest possible power consumption in the Host SoC.
The sample:

1. Initializes the Wi-Fi driver.
#. Scans for available Wi-Fi networks to verify that the Wi-Fi driver is operational.
#. Brings down the Wi-Fi network interface, which automatically directs the Wi-Fi driver to power down the nRF70 device.
#. Puts the Host SoC in the lowest possible power consumption mode.


User Interface
**************

Button 1:
   Wakes up the Host SoC, brings up the Wi-Fi network interface, which automatically directs the Wi-Fi driver to power on the nRF70 device.
   The sample then scans for available Wi-Fi networks to verify that the Wi-Fi driver is operational.

Button 2:
   Brings down the Wi-Fi network interface, which automatically directs the Wi-Fi driver to power down the nRF70 device.
   The Host SoC is put into the lowest possible power consumption mode.


Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/shutdown`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7000 EK, use the ``nrf5340dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command to demonstrate Wi-Fi shutdown:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek_nrf7000

Disable auto-start of the Wi-Fi driver
--------------------------------------

The Wi-Fi network interface is automatically brought up when the Wi-Fi driver is initialized by default.
You can disable it by setting the :kconfig:option:`CONFIG_NRF_WIFI_IF_AUTO_START` Kconfig option to ``n``.

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek_nrf7000 -DCONFIG_NRF_WIFI_IF_AUTO_START=n

With this configuration, the Wi-Fi network interface is not automatically brought up by the Zephyr networking stack.
You must press **Button 1** to bring up the Wi-Fi network interface.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      *** Booting Zephyr OS build v3.3.99-ncs1-26-ge405279d2134 ***
      [00:00:00.440,460] <inf> wifi_nrf: Firmware (v1.2.8.1) booted successfully

      [00:00:00.638,397] <inf> scan: Starting nrf5340dk_nrf5340_cpuapp with CPU frequency: 64 MHz

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
