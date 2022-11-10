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

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_security`
