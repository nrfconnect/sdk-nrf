.. _ble_nrf_dm:

Bluetooth: nRF Distance Measurement with Bluetooth LE discovery
###############################################################

.. contents::
   :local:
   :depth: 2

The nRF Distance Measurement sample demonstrates the functionality of the :ref:`mod_dm` subsystem.
It shows how to use DM to measure the distance between devices.
The Bluetooth® :ref:`ddfs_readme` is running simultaneously.

Sample is configured to use Nordic's SoftDevice link layer.

.. note::
   The Distance Measurement support in the |NCS| is :ref:`experimental <software_maturity>`.
   See :ref:`mod_dm` for details.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample initializes and performs distance measurements between devices.
The procedure for distance measurement on both devices must be synchronized.

The synchronization step has two goals:

   * Detect peers that support the ranging procedure.
   * Provide synchronization for the ranging step.

This sample synchronizes devices using the scan and advertising facilities of the Bluetooth LE.
Anchoring occurs after the exchange of ``SCAN_REQ`` and ``SCAN_RSP`` packets.

.. msc::
   hscale = "1.3";
   DevA [label="Device A"],DevB [label="Device B"];
   |||;
   DevA rbox DevB [label="Synchronization starts"];
   |||;
   DevA=>DevB [label="ADV_IND"];
   DevA<=DevB [label="SCAN_REQ"];
   DevA=>DevB [label="SCAN_RSP, Manufacturer-specific data"];
   |||;
   DevA rbox DevB [label="Synchronization ends"];
   |||;
   DevA box DevA [label="Reflector"],DevB box DevB [label="Initiator"];
   |||;
   DevA<=>DevB [label="Ranging"];

The chart shows the sequence of the synchronization process.

Both devices act as an advertiser and a scanner.
Device B receives advertising from Device A and scans it.
Device A sends a scan response that contains ``Manufacturer Specific Data``.
This message contains the following fields:

   * Company Identifier Code.
   * Identifier indicating that the device supports distance measurement.
   * The access address used for measurement.

This scanner/advertiser interaction is used as a synchronization point.
After a configurable delay, Device A and Device B try to range each other.

   * If a Device acts as an advertiser in synchronization, it will act as the reflector in the ranging procedure.
   * If a Device acts as a scanner in synchronization, it will act as the initiator in the ranging procedure.

The synchronization will be performed cyclically.

Based on the ``SCAN_RSP`` response, the device recognizes peers that support the distance measurement functionality within its coverage.
The addresses of these devices are stored in a list.
When a device does not respond within a certain period of time, it is removed from the list.
Also the last measurement results with the peers are stored in this list.

User interface
**************

LED 1:
   Indicates the measured distance for MCPD (Multi-carrier phase difference) method. The shorter the distance, the brighter the light.

LED 2:
   Blinks every two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

LED 3:
   On when connected.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/nrf_dm`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to development kits, test it by performing the following steps:

1. |connect_terminal_specific|
#. Reset the kit.
#. Program the other development kit with the same sample and reset it.
#. Wait until the devices identify themselves and synchronize.
#. Observe that the received measurement results are output in the terminal window.

Sample output
=============

The result should look similar to the following output:

* The MCPD method::

   *** Booting Zephyr OS build v2.6.99-ncs1  ***
   Starting Distance Measurement example
   I: SoftDevice Controller build revision:
   I: 3f 47 70 8e 81 95 4e 86 |?Gp...N.
   I: 9d d3 a2 95 88 f6 30 0a |......0.
   I: 7f 53 49 fd             |.SI.
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 63.28743 Build 1318420878
   I: Identity: C4:90:D5:4E:C2:20 (random)
   I: HCI: version 5.2 (0x0b) revision 0x125b, manufacturer 0x0059
   I: LMP: version 5.2 (0x0b) subver 0x125b
   DM Bluetooth LE Synchronization initialization

   Measurement result:
         Addr DB:49:E9:62:DF:6D (random)
         Quality ok
         Distance estimates: mcpd: ifft=0.29 phase_slope=0.28 rssi_openspace=0.28 best=0.29

   Measurement result:
         Addr CF:4E:38:D5:C0:ED (random)
         Quality ok
         Distance estimates: mcpd: ifft=0.94 phase_slope=1.10 rssi_openspace=0.22 best=0.94

   Measurement result:
         Addr CF:4E:38:D5:C0:ED (random)
         Quality ok
         Distance estimates: mcpd: ifft=0.79 phase_slope=1.08 rssi_openspace=0.25 best=0.79

* The RTT method::

   *** Booting Zephyr OS build v2.6.99-ncs1  ***
   Starting Distance Measurement example
   I: SoftDevice Controller build revision:
   I: 3f 47 70 8e 81 95 4e 86 |?Gp...N.
   I: 9d d3 a2 95 88 f6 30 0a |......0.
   I: 7f 53 49 fd             |.SI.
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 63.28743 Build 1318420878
   I: Identity: C4:90:D5:4E:C2:20 (random)
   I: HCI: version 5.2 (0x0b) revision 0x125b, manufacturer 0x0059
   I: LMP: version 5.2 (0x0b) subver 0x125b
   DM Bluetooth LE Synchronization initialization

   Measurement result:
         Addr CF:4E:38:D5:C0:ED (random)
         Quality ok
         Distance estimates: rtt: rtt=1.75

   Measurement result:
         Addr CF:4E:38:D5:C0:ED (random)
         Quality ok
         Distance estimates: rtt: rtt=1.70

   Measurement result:
         Addr DB:49:E9:62:DF:6D (random)
         Quality ok
         Distance estimates: rtt: rtt=2.75

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`ddfs_readme`
* :ref:`dk_buttons_and_leds_readme`

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_dm`

In addition, it uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:pwm_api`:

  * ``drivers/pwm.h``

* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :file:`include/random/rand32.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/scan.h`
* ``ext/hal/nordic/nrfx/hal/nrf_radio.h``
