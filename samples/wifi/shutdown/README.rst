.. _wifi_shutdown_sample:

Wi-Fi: Shutdown
###############

.. contents::
   :local:
   :depth: 2

The Shutdown sample demonstrates how to put the Nordic Semiconductor's Wi-Fi® chipset in the Shutdown state, where the device is completely powered off.
For more information, see the `nRF70 Series power states`_ page.

This also demonstrates how to achieve the lowest possible power consumption in the Host SoC (nRF53, nRF52 or nRF91 Series) when Wi-Fi is enabled but not being used.

The sample supports three operation modes:

* **Continuous mode (Default)**: Continuously cycles between Wi-Fi startup and shutdown with configurable timeout
* **One-shot mode**: Performs Wi-Fi startup followed by shutdown once, then remains in shutdown mode
* **Buttons mode**: Manual control of Wi-Fi startup and shutdown using buttons

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample can demonstrate Wi-Fi shutdown and achieve the lowest possible power consumption in the Host SoC.

In **Continuous mode** (default), the sample:

1. Initializes the Wi-Fi driver and powers up the nRF70 device.
#. Scans for available Wi-Fi networks to verify that the Wi-Fi driver is operational.
#. Brings down the Wi-Fi network interface, which automatically directs the Wi-Fi driver to power down the nRF70 device.
#. Puts the Host SoC in the lowest possible power consumption mode.
#. Waits for the configured timeout period (``SHUTDOWN_TIMEOUT_S``).
#. Repeats the cycle from step 1.

In **One-shot mode**, the sample performs steps 1-4 once and remains in shutdown mode permanently.

In **Buttons mode**, Functionality will be as per user button interaction to control the Wi-Fi state.

User Interface
**************

The user interface depends on the selected operation mode:

**Buttons Mode Only:**

Button 1:
   Wakes up the Host SoC, brings up the Wi-Fi network interface, which automatically directs the Wi-Fi driver to power on the nRF70 device.
   The sample then scans for available Wi-Fi networks to verify that the Wi-Fi driver is operational.

Button 2:
   Brings down the Wi-Fi network interface, which automatically directs the Wi-Fi driver to power down the nRF70 device.
   The Host SoC is put into the lowest possible power consumption mode.

**Continuous and One-shot Modes:**
   No user interaction required. The sample operates automatically according to the selected mode.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/shutdown`

.. include:: /includes/build_and_run_ns.txt

Operation Modes
===============

Continuous Mode (Default)
-------------------------

In continuous mode, the sample continuously cycles between Wi-Fi startup and shutdown with a configurable timeout period.

**Example for nRF7002 DK:**

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp

**Example for nRF7000 EK with nRF5340 DK:**

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek_nrf7000

One-shot Mode
-------------

In one-shot mode, the sample performs Wi-Fi startup and shutdown once, then remains in shutdown mode permanently.

**Example for nRF7002 DK:**

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_OPERATION_MODE_ONE_SHOT=y -DCONFIG_NRF_WIFI_IF_AUTO_START=n

**Example for nRF7000 EK with nRF5340 DK:**

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek_nrf7000 -DCONFIG_OPERATION_MODE_ONE_SHOT=y -DCONFIG_NRF_WIFI_IF_AUTO_START=n

Buttons Mode
------------

In buttons mode, Wi-Fi startup and shutdown are controlled manually using buttons. This mode is only verified on boards with nRF5340 host.

**Example for nRF7002 DK:**

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_OPERATION_MODE_BUTTONS=y

**Example for nRF7000 EK with nRF5340 DK:**

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek_nrf7000 -DCONFIG_OPERATION_MODE_BUTTONS=y

Configuration Options
=====================

Disable auto-start of the Wi-Fi driver
--------------------------------------

The Wi-Fi network interface is automatically brought up when the Wi-Fi driver is initialized by default.
You can disable it by setting the :kconfig:option:`CONFIG_NRF_WIFI_IF_AUTO_START` Kconfig option to ``n``.
This is automatically set for one-shot mode.

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_NRF_WIFI_IF_AUTO_START=n

Adjust shutdown timeout
-----------------------

In continuous mode, you can configure the timeout period between shutdown and restart cycles using the ``SHUTDOWN_TIMEOUT_S`` option (in seconds):

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_SHUTDOWN_TIMEOUT_S=10

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

Continuous Mode Testing
-----------------------

The sample shows the following output for continuous mode:

.. code-block:: console

   *** Booting nRF Connect SDK v3.0.99-7c4f8113c3e4 ***
   *** Using Zephyr OS v4.1.99-eb12c854d528 ***
   [00:00:00.428,253] <inf> shutdown: Starting nrf7002dk with CPU frequency: 64 MHz

   [00:00:00.431,640] <inf> shutdown: Scan requested

   Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
   1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
   2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
   3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
   [00:00:05.116,943] <inf> shutdown: Scan request done

   [00:00:05.130,706] <inf> shutdown: Interface down
   [00:00:05.297,180] <inf> shutdown: Interface up
   [00:00:05.299,194] <inf> shutdown: Scan requested

   Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
   1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
   2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
   3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
   [00:00:10.009,399] <inf> shutdown: Scan request done

   [00:00:10.022,735] <inf> shutdown: Interface down
   [00:00:15.189,270] <inf> shutdown: Interface up
   [00:00:15.191,284] <inf> shutdown: Scan requested

One-shot Mode Testing
---------------------

The sample shows the following output for one-shot mode:

.. code-block:: console

   *** Booting nRF Connect SDK v3.0.99-7c4f8113c3e4 ***
   *** Using Zephyr OS v4.1.99-eb12c854d528 ***
   [00:00:00.261,718] <inf> shutdown: Starting nrf7002dk with CPU frequency: 64 MHz

   [00:00:00.428,802] <inf> shutdown: OTP not programmed, proceeding with local MAC: F6:CE:36:00:00:01
   [00:00:00.430,877] <inf> shutdown: Scan requested

   Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
   1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
   2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
   3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
   [00:00:05.144,104] <inf> shutdown: Scan request done

   [00:00:05.157,714] <inf> shutdown: Interface down

Buttons Mode Testing
--------------------

For buttons mode testing (nRF5340 host boards only):

#. The sample starts with Wi-Fi initialization and shutdown.

#. Press **Button 1** to wake up the nRF5340 SoC, initialize the Wi-Fi chipset, and scan for available Wi-Fi networks:

   The sample shows the following output:

   .. code-block:: console

      [00:00:29.141,357] <inf> shutdown: Firmware (v1.2.8.1) booted successfully

      [00:00:29.269,165] <inf> shutdown: Interface up
      [00:00:29.272,521] <inf> shutdown: Scan requested

      Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
      1    | abcdef                           6     | 1    | -37  | WPA/WPA2 | aa:aa:aa:aa:aa:aa
      2    | pqrst                            5     | 1    | -65  | WPA/WPA2 | xx:xx:xx:xx:xx:xx
      3    | AZBYCXD                          7     | 1    | -41  | WPA/WPA2 | yy:yy:yy:yy:yy:yy
      [00:00:34.092,285] <inf> shutdown: Scan request done

#. Press **Button 2** to shut down the Wi-Fi driver and put the nRF5340 SoC in lowest possible power consumption mode:

   The sample shows the following output:

   .. code-block:: console

      [00:00:48.313,354] <inf> shutdown : Interface down
