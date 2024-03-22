.. _bluetooth_isochronous_combined_bis_and_cis:

Bluetooth: ISO Combined BIS and CIS
#####################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Combined BIS and CIS sample demonstrates capability of
a Controller to support both Broadcast Isochronous Stream (BIS) and Connected Isochronous Stream (CIS) and
can be used to learn ISO-specific APIs to create stream, receive and send data over either CIS or BIS.
Note: the sample uses the simplest way to send data over ISO without using timestamps through a single stream,
therefore it doesn't demonstrate any capabilities of synchronization multiple streams.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

You can use any of the development kits listed above and mix different development kits.

.. include:: /includes/hci_ipc_overlay.txt

The sample also requires a connection to a computer with a serial terminal |ANSI| for each of the development kits.

Overview
********

The sample demonstrates a device combining CIS Central and BIS Source.
As a CIS Central, it only implements receiving of data from CIS Peripheral.
In order to provide the best performance and minimize number of conflicts between CIS, BIS and ACL packets the following measures are taken:
* The same ISO Interval is used for both CIS and BIS. The value is set to 10 ms by default.
* ACL Interval is set to integer multiple of ISO Interval to avoid collisions. The value is set to 20 ms by default.
* Periodic advertising of BIS is set to the same value as ACL Interval.
* Number of retransmissions is set to a high value - 10, to ensure that packets are retransmitted in case of collisions.
* The sample as Central rejects incoming Connection Update requests to avoid changing the ACL Interval value.
* In addition the following optional SDC-specific configurations are used:
   * CONFIG_BT_CTLR_SDC_CENTRAL_ACL_EVENT_SPACING_DEFAULT is set to 10 ms to ensure that periodic advertising packet is set in between ACL packets.
   * CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT is set to a small value 2.5 ms because the sample doesn't send any data over ACL connection (except Control procedures).


Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/iso_combined_bis_and_cis`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

The sample requires two devices in addition:
* CIS Peripheral device that sends data over CIS.
* BIS Sink device that receives data over BIS.
Other sample application can be used as CIS Peripheral and BIS Sink.
CIS Peripheral device should have the same name as the name set in CONFIG_BT_DEVICE_NAME.

After programming the sample to the development kit and programming other samples to CIS Peripheral device and BIS sink device, perform the following steps to test it:

1. |connect_terminal_specific|
#. Reset the kits.
#. Wait until the sample connected to CIS Peripheral device and established CIS connection.
#. Observe packets transmitted by CIS Peripheral to be retransmitted over BIS and received by BIS Sink.


Sample output
==============

[00:00:00.454,589] <inf> bt_hci_core: HW Platform: Nordic Semiconductor (0x0002)
[00:00:00.454,620] <inf> bt_hci_core: HW Variant: nRF53x (0x0003)
[00:00:00.454,650] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00) Version ... Build ...
[00:00:00.456,329] <inf> bt_hci_core: Identity: FD:DD:D6:88:AF:D4 (random)
[00:00:00.456,359] <inf> bt_hci_core: HCI: version 5.4 (0x0d) revision ..., manufacturer 0x0059
[00:00:00.456,390] <inf> bt_hci_core: LMP: version 5.4 (0x0d) subver ...
[00:00:00.456,390] <inf> app_main:
Bluetooth ISO Combined BIS and CIS sample

The sample demonstrates data transfer over Bluetooth ISO
CIS and BIS using the following topology:

┌------┐     ┌-------┐      ┌--------┐
|      |ISO  |  CIS  | ISO  |        |
| CIS  ├----►|Central├-----►|BIS Sink|
|Periph|CIS  |+ BIS  | BIS  |        |
|      |     |Source |      |        |
└------┘     └-------┘      └--------┘
  (1)           (2)            (3)
The sample only operates as a device 2: Combined CIS Central + BIS Source
Please, use other samples as a CIS Peripheral (TX) and BIS Sink (RX) devices.


[00:00:00.456,420] <inf> app_main: Starting combined CIS Central + BIS Source
[00:00:00.459,777] <inf> app_bis_cis: CIS central started scanning for peripheral(s)
[00:00:00.593,078] <inf> app_bis_cis: Connected: FB:DC:75:C0:18:0A (random)
[00:00:00.593,597] <inf> app_bis_cis: Connecting ISO channel
[00:00:00.600,189] <inf> app_bis_cis: BIS transmitter started
[00:00:00.607,879] <inf> app_iso_tx: ISO TX Channel connected
[00:00:00.608,459] <inf> app_iso_tx: Sent SDU, counter: 0
[00:00:00.741,577] <inf> app_iso_rx: ISO RX Channel connected
[00:00:01.449,615] <inf> app_iso_tx: Sent SDU, counter: 100
[00:00:01.731,140] <inf> app_iso_rx: Received SDU: 100, empty SDU: 0, missed SDU: 0
[00:00:02.449,645] <inf> app_iso_tx: Sent SDU, counter: 200
[00:00:02.731,140] <inf> app_iso_rx: Received SDU: 200, empty SDU: 0, missed SDU: 0
[00:00:03.449,615] <inf> app_iso_tx: Sent SDU, counter: 300
[00:00:03.731,140] <inf> app_iso_rx: Received SDU: 300, empty SDU: 0, missed SDU: 0
[00:00:04.449,615] <inf> app_iso_tx: Sent SDU, counter: 400
[00:00:04.731,140] <inf> app_iso_rx: Received SDU: 400, empty SDU: 0, missed SDU: 0
[00:00:05.449,645] <inf> app_iso_tx: Sent SDU, counter: 500


References
***********

For more information about the scheduling of different types of packets refer to the `SDC Scheduling Documentation`_.