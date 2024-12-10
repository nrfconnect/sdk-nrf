.. _bluetooth_iso_combined_bis_cis:

Bluetooth: ISO combined BIS and CIS
###################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® ISO combined BIS and CIS sample demonstrates the capability of a Controller to support both Broadcast Isochronous Stream (BIS) and Connected Isochronous Stream (CIS) simultaneously when using the LE Isochronous Channels (ISO) feature.
It uses ISO-specific APIs to create a stream, and to receive and send data.
The sample acts as a Central device that receives data over CIS and forwards the received data to a BIS.

.. note::
   The sample uses the Time of Arrival method to send data over ISO without using timestamps through a single stream (see the :ref:`nrfxlib:iso_providing_data` section in the SoftDevice Controller documentation for more information).
   The :ref:`bluetooth_isochronous_time_synchronization` sample can be used to learn how to synchronize data using ISO channels.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

To test the sample, you need two additional devices.
You can use any of the development kits listed above and mix different development kits.
The sample also requires a connection to a computer with a serial terminal |ANSI| for each of the development kits.

Overview
********

The sample demonstrates a device combining the CIS Central and BIS Source roles.
As a CIS Central, it only receives data from a CIS Peripheral.
The received data is then broadcasted over a BIS.
To provide the best performance and to minimize the number of conflicts between the CIS, BIS and ACL roles, the following measures are taken:

* The same ISO interval is used for both CIS and BIS.
  The value is set to 10 ms by default.
* ACL interval is set to an integer multiple of ISO interval to avoid collisions.
  The value is set to 20 ms by default.
* Periodic advertising of the BIS is set to the same value as the ACL interval.
* The number of retransmissions is set to 10, to ensure that packets are retransmitted in case of collisions.
* The sample rejects incoming Connection Update requests from the peripheral to avoid changes to the ACL interval.
* The following optional Kconfig options specific to the SoftDevice Controller are used:

   * :kconfig:option:`CONFIG_BT_CTLR_SDC_CENTRAL_ACL_EVENT_SPACING_DEFAULT` is set to 10 ms to ensure that there is enough time available to place the periodic advertising packet between ACL packets.
   * :kconfig:option:`CONFIG_BT_CTLR_SDC_MAX_CONN_EVENT_LEN_DEFAULT` is set to a small value 2.5 ms because the only data the sample sends over the ACL connection is Control procedures.
   * :kconfig:option:`CONFIG_BT_CTLR_SDC_BIG_RESERVED_TIME_US` is set to half of the ISO interval to ensure the BIG parameters are selected in a way that leaves enough time for ACL and CIG.
   * :kconfig:option:`CONFIG_BT_CTLR_SDC_CIG_RESERVED_TIME_US` is set to half of the ISO interval to ensure the CIG parameters are selected in a way that leaves enough time for ACL and BIG.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/iso_combined_bis_and_cis`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

In addition to the device running this sample, two additional devices are required:

* CIS Peripheral device that sends data over CIS.
* BIS Sink device that receives data over BIS.

You can use other samples, such as the :ref:`bluetooth_isochronous_time_synchronization` sample, as the CIS Peripheral and BIS Sink.
The CIS Peripheral device should have the same name as the name set in the :kconfig:option:`CONFIG_BT_DEVICE_NAME` Kconfig option of this sample.
This name is used to connect to the correct device.

After programming the sample to the development kit and programming other samples to the CIS Peripheral device and BIS sink device, perform the following steps to test it:

1. |connect_terminal_specific|
#. Reset the kits.
#. Wait until the sample connects to CIS Peripheral device and establishes a CIS Peripheral device and established CIS connection.
#. Observe the packets transmitted by the CIS Peripheral being retransmitted over BIS and received by the BIS Sink.


Sample output
=============

The serial output should look similar to the following output:

.. code-block:: console

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


Dependencies
************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:softdevice_controller`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :file:`include/sys/printk.h`
* :file:`include/sys/ring_buffer.h`
* :file:`include/zephyr/types.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/iso.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/scan.h`

References
**********

For more information about the scheduling of different types of packets, refer to the :ref:`softdevice_controller_scheduling`.
