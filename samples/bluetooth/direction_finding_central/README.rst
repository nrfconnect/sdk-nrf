.. _bluetooth_direction_finding_central:

Bluetooth: Direction finding central
####################################

.. contents::
   :local:
   :depth: 2

The direction finding central sample application demonstrates Bluetooth® LE direction finding in a response received from a connected peripheral device.

Requirements
************

.. bt_dir_finding_central_req_start

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires an antenna matrix when operating in angle-of-arrival mode.
It can be a Nordic Semiconductor design 12 patch antenna matrix, or any other antenna matrix.

.. bt_dir_finding_central_req_end

Overview
********

The direction finding central sample application uses Constant Tone Extension (CTE), received in a request response from a connected peer device.

.. bt_dir_finding_central_ov_start

The sample supports two direction finding modes:

* Angle of Arrival (AoA)
* Angle of Departure (AoD)

By default, both modes are available in the sample.

.. bt_dir_finding_central_ov_end

Configuration
*************

.. bt_dir_finding_central_conf_start

|config|

This sample configuration is split into the following two files:

* Generic configuration available in the :file:`prj.conf` file
* Board-specific configuration available in the :file:`boards/<BOARD>.conf` file

.. bt_dir_finding_central_conf_end

.. bt_dir_finding_central_5340_conf_start

nRF5340 configuration files
===========================

The following additional configuration files are available for the :ref:`nRF5340 DK <ug_nrf5340>`:

* The Bluetooth LE controller is part of an image meant to run on the network core.
  The configuration for the image is stored in the :file:`sysbuild/` subdirectory.
* The DTS overlay file :file:`boards/nrf5340dk_nrf5340_cpuapp.overlay` is available for the application core.
  This file forwards the control over GPIOs to network core, which provides control over GPIOs to the radio peripheral in order to execute antenna switching.

.. bt_dir_finding_central_5340_conf_end

.. bt_dir_finding_central_aod_start

Angle of departure mode
=======================

To build this sample with AoD mode only, set :makevar:`EXTRA_CONF_FILE` to the :file:`overlay-aod.conf` file using the respective :ref:`CMake option <cmake_options>`.

For more information about configuration files in the |NCS|, see :ref:`app_build_system`.

To build this sample for the :ref:`nRF5340 DK <ug_nrf5340>` with AoD mode only, add the content of the :file:`overlay-aod.conf` file to the :file:`sysbuild/hci_ipc/prj.conf` file.

.. bt_dir_finding_central_aod_end

.. bt_dir_finding_central_ant_aoa_start

Antenna matrix configuration for angle of arrival mode
======================================================

To use this sample when AoA mode is enabled, additional configuration of GPIOs is required to control the antenna array.
An example of such configuration is provided in a devicetree overlay file :file:`nrf52833dk_nrf52833.overlay`.

The overlay file provides the information of which GPIOs should be used by the Radio peripheral to switch between antenna patches during the CTE reception in the AoA mode.
At least two GPIOs must be provided to enable antenna switching.

The GPIOs are used by the Radio peripheral in the order provided by the ``dfegpio#-gpios`` properties.
The order is important, because it has an impact on the mapping of the antenna switching patterns to GPIOs (see `Antenna patterns`_).

.. bt_dir_finding_central_ant_aoa_end

To successfully use the direction finding central when the AoA mode is enabled, provide the following data related to antenna matrix design:

.. bt_dir_finding_central_conf_list_start

* The GPIO pins to ``dfegpio#-gpios`` properties in the :file:`nrf52833dk_nrf52833.overlay` file.
* The default antenna that will be used to receive a PDU ``dfe-pdu-antenna`` property in the :file:`nrf52833dk_nrf52833.overlay` file.
* Update the antenna switching patterns of the :c:member:`ant_patterns` array in the :file:`main.c` file.

.. bt_dir_finding_central_conf_list_end

.. bt_dir_finding_central_ant_pat_start

Antenna patterns
================

The antenna switching pattern is a binary number where each bit is applied to a particular antenna GPIO pin.
For example, the pattern ``0x3`` means that antenna GPIOs at index ``0,1`` will be set, while the following are left unset.

This also means that, for example, when using four GPIOs, the pattern count cannot be greater than 16 and the maximum allowed value is 15.

If the number of switch-sample periods is greater than the number of stored switching patterns, the radio loops back to the first pattern.

The length of the antenna switching pattern is limited by the :kconfig:option:`CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN` Kconfig option.
If the required length of the antenna switching pattern is greater than the default value of that option, set it to the required value in the board configuration file.
For example, for the :ref:`nRF52833 DK <ug_nrf52>`, set the option value to the required antenna switching pattern length in the :file:`nrf52833dk_nrf52833.conf` file.

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

.. bt_dir_finding_central_ant_pat_end

.. bt_dir_finding_central_cte_start

Constant Tone Extension transmit and receive parameters
=======================================================

Constant Tone Extension works in either AoA or AoD mode.
The transmitter is configured with operation mode that is used for CTE transmission.
The receiver operating mode depends on the configuration.
By default, both AoA and AoD modes are enabled, and the CTE type is received in the ``CTEInfo`` field in PDU's extended advertising header.
Depending on the operation mode selected for the CTE reception, the receiver's radio peripheral will either execute both antenna switching and CTE sampling (AoA), or CTE sampling only (AoD).

The allowed antenna switch slot lengths are 1 µs or 2 µs.
The transmitter uses the antenna switch slot length to configure the radio peripheral in AoD mode only.
The receiver uses the local setting of the antenna switch slot length in AoA mode only.
When the CTE type in the received PDU is AoD, the receiver's radio peripheral uses the antenna switch slot length appropriate for the AoD type (1 or 2 µs).
The antenna switch slot length has an impact on the number of IQ samples provided in the IQ samples report.

Constant Tone Extension length is limited to a range between 16 µs and 160 µs.
The value is provided in units of 8 µs.
The transmitter is responsible for setting the CTE length.
The value is sent to the receiver as part of the CTEInfo field in PDU's extended advertising header.
The receiver's radio peripheral uses the CTE length, provided in periodic advertising PDU, to execute antenna switching and CTE sampling in AoA mode, or CTE sampling only in AoD mode.
The CTE length has an impact on the number of IQ samples provided in the IQ samples report.

Constant Tone Extension consists of the following three periods:

* Guard period of 4 µs. There is a gap before the actual CTE reception and the adjacent PDU transmission to avoid interference.
* Reference period where a single antenna is used for sampling. The spacing between the samples is 1 µs and the period duration is 8 µs.
* Switch-sample period that is split into switch and sample slots. The length of each slot can be either 1 or 2 µs.

The total number of IQ samples provided by the IQ samples report varies.
It depends on the CTE length and the antenna switch slot length.
There will always be eight samples from the reference period, 1 to 37 samples with 2 µs antenna switching slots, 2 to 74 samples with 1 µs antenna switching slots, totalling in 9 to 82 samples.

For example, the CTE length is 120 µs and the antenna switching slot is 1 µs.
This will result in the following number of IQ samples:

* Eight samples from the reference period.
* Switch-sample period duration is ``120 µs - 12 µs = 108 µs``. For the 1 µs antenna switching slot, the sample is taken every 2 µs (1 µs antenna switch slot, 1 µs sample slot). In this case, the number of samples from the switch-sample period is ``108 µs / 2 µs = 54``.

The total number of samples is 62.

.. bt_dir_finding_central_cte_end

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/direction_finding_central`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

|test_sample|

1. |connect_terminal_specific|
#. In the terminal window, check for information similar to the following::

      Bluetooth initialized
      <inf> bt_hci_core: HW Platform: Nordic Semiconductor (XxXXXX)
      <inf> bt_hci_core: HW Variant: nRF52x (XxXXXX)
      <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0xXX) Version 3.0 Build X
      <inf> bt_hci_core: Identity: XX:XX:XX:XX:XX:XX (random)
      <inf> bt_hci_core: HCI: version 5.3 (XxXX) revision XxXXXX, manufacturer XxXXXX
      <inf> bt_hci_core: LMP: version 5.3 (XxXX) subver XxXXXX
      Scanning successfully started
      [DEVICE]: XX:XX:XX:XX:XX:XX (public), AD evt type 0, AD data len XX, RSSI XXX
      [AD]: X data_len X
      [AD]: X data_len X
      Connected: XX:XX:XX:XX:XX:XX (random)
      Enable receiving of CTE...
      success. CTE receive enabled.
      Request CTE from peer device...
      success. CTE request enabled.
      CTE[XX:XX:XX:XX:XX:XX (random)]: samples count XX, cte type XXX, slot durations: X [us], packet status XXX , RSSI XXX

Dependencies
************

This sample uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :file:`include/sys/util.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/direction.h`
  * :file:`include/bluetooth/conn.h`
