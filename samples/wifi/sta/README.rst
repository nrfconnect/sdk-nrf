.. _wifi_station_sample:

Wi-Fi: Station
##############

.. contents::
   :local:
   :depth: 2

The Station sample demonstrates how to connect the Wi-Fi station to a specified access point using Dynamic Host Configuration Protocol (DHCP).

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample can perform Wi-Fi operations such as connect and disconnect in the 2.4GHz and 5GHz bands depending on the capabilities of an access point.

Using this sample, the development kit can connect to the specified access point in :abbr:`STA (Station)` mode.

Quad Serial Peripheral Interface (QSPI) encryption
**************************************************

This sample demonstrates QSPI encryption API usage, the key can be set by the :kconfig:option:`CONFIG_NRF700X_QSPI_ENCRYPTION_KEY` Kconfig option.

If encryption of the QSPI traffic is required for the production devices, then matching keys must be programmed in both the nRF7002 OTP and non-volatile storage associated with the host.
The key from non-volatile storage must be set as the encryption key using the APIs.

Power management
****************

This sample also enables Zephyr's power management policy by default, which puts the nRF5340 :term:`System on Chip (SoC)` into low-power mode whenever it is idle.
See :ref:`zephyr:pm-guide` in the Zephyr documentation for more information on power management.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/sta`

.. include:: /includes/build_and_run_ns.txt

Currently, the following board(s) are supported:

* nRF7002 DK

You must configure the following Wi-Fi credentials in ``prj.conf``:

* Network name (SSID)
* Key management
* Password

.. note::
   You can also use ``menuconfig`` to enable ``Key management`` option.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

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

      <inf> sta: Connection requested

      <inf> sta: ==================
      <inf> sta: State: SCANNING
      <inf> sta: ==================
      <inf> sta: State: SCANNING
      <inf> sta: ==================
      <inf> sta: State: SCANNING
      <inf> sta: ==================
      <inf> sta: State: ASSOCIATING
      <inf> sta: ==================
      <inf> sta: State: ASSOCIATED
        <inf> sta: Interface Mode: STATION
        <inf> sta: Link Mode: WIFI 6 (802.11ax/HE)
        <inf> sta: SSID: ABCDEFGH
        <inf> sta: BSSID: F0:1D:2D:72:EB:EF
        <inf> sta: Band: 5GHz
        <inf> sta: Channel: 108
        <inf> sta: Security: WPA2-PSK
        <inf> sta: MFP: Optional
        <inf> sta: RSSI: -47
      <inf> sta: ==================
      <inf> sta: State: 4WAY_HANDSHAKE
        <inf> sta: Interface Mode: STATION
        <inf> sta: Link Mode: WIFI 6 (802.11ax/HE)
        <inf> sta: SSID: ABCDEFGH
        <inf> sta: BSSID: F0:1D:2D:72:EB:EF
        <inf> sta: Band: 5GHz
        <inf> sta: Channel: 108
        <inf> sta: Security: WPA2-PSK
        <inf> sta: MFP: Optional
        <inf> sta: RSSI: -47
      <inf> sta: ==================
      <inf> sta: State: COMPLETED
        <inf> sta: Interface Mode: STATION
        <inf> sta: Link Mode: WIFI 6 (802.11ax/HE)
        <inf> sta: SSID: ABCDEFGH
        <inf> sta: BSSID: F0:1D:2D:72:EB:EF
        <inf> sta: Band: 5GHz
        <inf> sta: Channel: 108
        <inf> sta: Security: WPA2-PSK
        <inf> sta: MFP: Optional
        <inf> sta: RSSI: -47

      <inf> sta: Connected

      <inf> sta: ==================
      <inf> sta: State: COMPLETED
        <inf> sta: Interface Mode: STATION
        <inf> sta: Link Mode: WIFI 6 (802.11ax/HE)
        <inf> sta: SSID: ABCDEFGH
        <inf> sta: BSSID: F0:1D:2D:72:EB:EF
        <inf> sta: Band: 5GHz
        <inf> sta: Channel: 108
        <inf> sta: Security: WPA2-PSK
        <inf> sta: MFP: Optional
        <inf> sta: RSSI: -47

      <inf> sta: IP address: 192.168.50.237

      <inf> sta: Disconnect requested
      <inf> sta: Disconnection request done (0)
      <inf> sta: ==================
      <inf> sta: State: DISCONNECTED

Power management testing
************************

This sample can be used to measure current consumption of both the nRF5340 SoC and nRF7002 device independently by using two separate Power Profiler Kit II's (PPK2's).
The nRF5340 SoC is connected to the first PPK2 and the nRF7002 DK is connected to the second PPK2.

Hardware modifications
======================

To measure the current consumption of the nRF5340 SoC, complete the following steps:

 * Remove jumper on **P22** (VDD jumper).
 * Cut solder bridge **SB16**.
 * Make a short on solder bridge **SB17**.
 * Connect **GND** on PPK2 kit to **GND** on the nRF7002 DK.
    You can use **P21** pin **1** mentioned as **GND** (-).
 * Connect the **Vout** on PPK2 to **P22** pin **1** on the nRF7002 DK.

To measure the current consumption of the nRF7002 device, complete the following steps:

 * Remove jumper on **P23** (VBAT jumper).
 * Connect **GND** on PPK2 kit to **GND** on the nRF7002 DK.
    You can use **P21** pin **1** mentioned as  **GND** (-).
 * Connect the **Vout** on PPK2 to **P23** pin **1** on the nRF7002 DK.

PPK II usage and measurement
============================

See :ref:`app_power_opt` for more information on power management testing and usage of the PPK2.
The average current consumption in an idle case can be around ~1-2 mA in the nRF5340 SoC and ~20 µA in the nRF7002 DK.

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_security`
