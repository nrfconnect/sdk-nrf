.. _direction_finding_connectionless_tx:

Bluetooth: Direction finding connectionless beacon
##################################################

.. contents::
   :local:
   :depth: 2

Overview
********

The direction finding connectionless beacon sample demonstrates Bluetooth LE Direction Finding transmission.
It uses the Constant Tone Extension (CTE), that is transmitted with periodic advertising PDUs.

The sample supports two Direction Finding modes:

* Angle of Arrival
* Angle of Departure

By default, both modes are available in the sample.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52833dk_nrf52833

The sample also requires an antenna matrix when operating in Angle of Departure mode.
It can be a Nordic Semiconductor design 12 patch antenna matrix, or any other antenna matrix.

Configuration
*************

|config|

Angle of Arrival mode
=====================

To build this sample with Angle of Arrival mode only, set ``OVERLAY_CONFIG`` to :file:`overlay-aoa.conf`.

See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

Antenna matrix configuration for Angle of Departure mode
========================================================

To use this sample when Angle of Departure mode is enabled, additional configuration of GPIOs is required to control the antenna array.
Example of such configuration is provided in a devicetree overlay file :file:`nrf52833dk_nrf52833.overlay`.

The overlay file provides the information about which GPIOs should be used by the Radio peripheral to switch between antenna patches during the CTE transmission in the AoD mode.
At least two GPIOs must be provided to enable antenna switching.

The GPIOs are used by the Radio peripheral in order given by the ``dfegpio#-gpios`` properties.
The order is important because it affects mapping of the antenna switching patterns to GPIOs (see `Antenna patterns`_).

To successfully use the Direction Finding beacon when the AoD mode is enabled, provide the following data related to antenna matrix design:

* Provide the GPIO pins to ``dfegpio#-gpios`` properties in :file:`nrf52833dk_nrf52833.overlay` file
* Provide the default antenna that will be used to transmit PDU ``dfe-pdu-antenna`` property in :file:`nrf52833dk_nrf52833.overlay` file
* Update the antenna switching patterns in :c:member:`ant_patterns` array in :file:`main.c`.

Antenna patterns
================

The antenna switching pattern is a binary number where each bit is applied to a particular antenna GPIO pin.
For example, the pattern 0x3 means that antenna GPIOs at index 0,1 will be set, while the following are left unset.

This also means that, for example, when using four GPIOs, the pattern count cannot be greater than 16 and maximum allowed value is 15.

If the number of switch-sample periods is greater than the number of stored switching patterns, then the radio loops back to the first pattern.

The following table presents the patterns that you can use to switch antennas on the Nordic-designed antenna matrix:

+--------+--------------+
|Antenna | PATTERN[3:0] |
+========+==============+
| ANT_12 |  0 (0b0000)  |
+--------+--------------+
| ANT_10 |  1 (0b0001)  |
+--------+--------------+
| ANT_11 |  2 (0b0010)  |
+--------+--------------+
| RFU    |  3 (0b0011)  |
+--------+--------------+
| ANT_3  |  4 (0b0100)  |
+--------+--------------+
| ANT_1  |  5 (0b0101)  |
+--------+--------------+
| ANT_2  |  6 (0b0110)  |
+--------+--------------+
| RFU    |  7 (0b0111)  |
+--------+--------------+
| ANT_6  |  8 (0b1000)  |
+--------+--------------+
| ANT_4  |  9 (0b1001)  |
+--------+--------------+
| ANT_5  | 10 (0b1010)  |
+--------+--------------+
| RFU    | 11 (0b1011)  |
+--------+--------------+
| ANT_9  | 12 (0b1100)  |
+--------+--------------+
| ANT_7  | 13 (0b1101)  |
+--------+--------------+
| ANT_8  | 14 (0b1110)  |
+--------+--------------+
| RFU    | 15 (0b1111)  |
+--------+--------------+

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/direction_finding_connectionless_tx`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal_specific|
#. In the terminal window, check for information similar to the following::

      Starting Direction Finding periodic advertising Beacon Demo
      Bluetooth initialization...success
      Advertising set create...success
      Update CTE params...success
      Periodic advertising params set...success
      Enable CTE...success
      Periodic advertising enable...success
      Extended advertising enable...success
      Started extended advertising as XX:XX:XX:XX:XX:XX (random)

Dependencies
************

This sample uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``lib/libc/minimal/include/errno.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* ``include/sys/util.h``
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/direction.h``
  * ``include/bluetooth/gatt.h``
