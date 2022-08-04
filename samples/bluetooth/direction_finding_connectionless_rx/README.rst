.. _direction_finding_connectionless_rx:

Bluetooth: Direction finding connectionless locator
###################################################

.. contents::
   :local:
   :depth: 2

The direction finding connectionless locator sample application demonstrates Bluetooth® LE direction finding reception.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an antenna matrix when operating in angle of arrival mode.
It can be a Nordic Semiconductor design 12 patch antenna matrix, or any other antenna matrix.

Overview
********

The direction finding connectionless locator sample application uses Constant Tone Extension (CTE), that is received and sampled with periodic advertising PDUs.

The sample supports two direction finding modes:

* Angle of Arrival (AoA)
* Angle of Departure (AoD)

By default, both modes are available in the sample.

Configuration
*************

|config|

This sample configuration is split into the following two files:

* generic configuration is available in :file:`prj.conf` file
* board specific configuration is available in :file:`boards/<BOARD>.conf` file

nRF5340 configuration files
===========================

The following additional configuration files are available for the :ref:`nRF5340 DK <ug_nrf5340>`:

* The Bluetooth LE controller is part of a child image meant to run on the network core.
  The configuration for the child image is stored in the :file:`child_image/` subdirectory.
* :file:`boards/nrf5340dk_nrf5340_cpuapp.overlay` DTS overlay file is available for the application core.
  This file forwards the control over GPIOs to network core, which gives control over GPIOs to the radio peripheral in order to execute antenna switching.

Angle of departure mode
=======================

To build this sample with angle of departure mode only, set ``OVERLAY_CONFIG`` to :file:`overlay-aod.conf`.

See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

To build this sample for :ref:`nRF5340 DK <ug_nrf5340>`, with angle of arrival mode only, add content of :file:`overlay-aod.conf` file to :file:`child_image/hci_rpmsg.conf` file.

Antenna matrix configuration for angle of arrival mode
======================================================

To use this sample when angle of arrival mode is enabled, additional configuration of GPIOs is required to control the antenna array.
Example of such configuration is provided in a devicetree overlay file :file:`nrf52833dk_nrf52833.overlay`.

The overlay file provides the information about which GPIOs should be used by the Radio peripheral to switch between antenna patches during the CTE reception in the AoA mode.
At least two GPIOs must be provided to enable antenna switching.

The GPIOs are used by the Radio peripheral in order given by the ``dfegpio#-gpios`` properties.
The order is important because it affects mapping of the antenna switching patterns to GPIOs (see `Antenna patterns`_).

To successfully use the direction finding locator when the AoA mode is enabled, provide the following data related to antenna matrix design:

* Provide the GPIO pins to ``dfegpio#-gpios`` properties in :file:`nrf52833dk_nrf52833.overlay` file
* Provide the default antenna that will be used to receive PDU ``dfe-pdu-antenna`` property in :file:`nrf52833dk_nrf52833.overlay` file
* Update the antenna switching patterns in :c:member:`ant_patterns` array in :file:`main.c`.

Antenna patterns
================

The antenna switching pattern is a binary number where each bit is applied to a particular antenna GPIO pin.
For example, the pattern 0x3 means that antenna GPIOs at index 0,1 will be set, while the following are left unset.

This also means that, for example, when using four GPIOs, the pattern count cannot be greater than 16 and maximum allowed value is 15.

If the number of switch-sample periods is greater than the number of stored switching patterns, then the radio loops back to the first pattern.

The length of the antenna switching pattern is limited by the :kconfig:option:`CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN` option.
If the required length of the antenna switching pattern is greater than the default value of :kconfig:option:`CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN`, set the :kconfig:option:`CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN` option to the required value in the board configuration file.
For example, for the :ref:`nRF52833 DK <ug_nrf52>` add :kconfig:option:`CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN` =N, where N is the required antenna switching pattern length, to the :file:`nrf52833dk_nrf52833.conf` file.

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

Constant Tone Extension transmit and receive parameters
=======================================================

Constant Tone Extension works in one of two modes, angle of arrival (AoA) or angle of departure (AoD).
The transmitter is configured with operation mode that is used for CTE transmission.
The receiver operating mode depends on the configuration.
By default, both AoA and AoD modes are enabled, and the CTE type is received in the CTEInfo field in PDU's extended advertising header.
Depending on the operation mode selected for the CTE reception, the receiver's radio peripheral will either execute both antenna switching and CTE sampling (AoA), or CTE sampling only (AoD).

There are two antenna switch slot lengths allowed, 1 µs or 2 µs.
The transmitter uses the antenna switch slot length to configure radio peripheral in angle of departure mode only.
The receiver uses the local setting of the antenna switch slot length in angle of arrival mode only.
When the CTE type in the received PDU is AoD, the receiver's radio peripheral uses the antenna switch slot length appropriate for the AoD type (1 or 2 µs).
The antenna switch slot length influences the number of IQ samples provided in the IQ samples report.

Constant Tone Extension length is limited to a range between 16 µs and 160 µs.
The value is provided in units of 8 µs.
The transmitter is responsible for setting the CTE length.
The value is sent to the receiver as part of the CTEInfo field in PDU's extended advertising header.
The receiver's radio peripheral uses the CTE length, provided in periodic advertising PDU, to execute antenna switching and CTE sampling in AoA mode, or CTE sampling only in AoD mode.
The CTE length has influence on number of IQ samples provided in the IQ samples report.

Constant Tone Extension consists of three periods:

* Guard period that lasts for 4 µs. There is a gap before the actual CTE reception and the adjacent PDU transmission to avoid interference.
* Reference period where single antenna is used for sampling. The samples are spaced 1 µs from each other. The period duration is 8 µs.
* Switch-sample period that is split into switch and sample slots. Each slot may be either 1 µs or 2 µs long.

The total number of IQ samples provided by the IQ samples report varies.
It depends on the CTE length and the antenna switch slot length.
There will always be 8 samples from the reference period, 1 to 37 samples with 2 µs antenna switching slots, 2 to 74 samples with 1 µs antenna switching slots, meaning 9 to 82 samples in total.

For example, the CTE length is 120 µs and the antenna switching slot is 1 µs.
This will result in the following number of IQ samples:

* 8 samples from the reference period.
* Switch-sample period duration is: 120 µs - 12 µs = 108 µs. For the 1 µs antenna switching slot, the sample is taken every 2 µs (1 µs antenna switch slot, 1 µs sample slot). Meaning, the number of samples from the switch-sample period is 108 µs / 2 µs = 54.

The total number of samples is 62.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/direction_finding_connectionless_rx`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. In the terminal window, check for information similar to the following::


      Starting Connectionless Locator Demo
      Bluetooth initialization...success
      Scan callbacks register...success.
      Periodic Advertising callbacks register...success.
      Start scanning...success
      Waiting for periodic advertising...
      [DEVICE]: XX:XX:XX:XX:XX:XX, AD evt type X, Tx Pwr: XXX, RSSI XXX C:X S:X D:X SR:X E:1 Prim: XXX, Secn: XXX, Interval: XXX (XXX ms), SID: X
      success. Found periodic advertising.
      Creating Periodic Advertising Sync...success.
      Waiting for periodic sync...
      PER_ADV_SYNC[0]: [DEVICE]: XX:XX:XX:XX:XX:XX synced, Interval XXX (XXX ms), PHY XXX
      success. Periodic sync established.
      Enable receiving of CTE...
      success. CTE receive enabled.
      Scan disable...Success.
      Waiting for periodic sync lost...
      PER_ADV_SYNC[X]: [DEVICE]: XX:XX:XX:XX:XX:XX, tx_power XXX, RSSI XX, CTE XXX, data length X, data: XXX
      CTE[X]: samples count XX, cte type XXX, slot durations: X [us], packet status XXX, RSSI XXX


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
  * ``include/bluetooth/direction.h``
