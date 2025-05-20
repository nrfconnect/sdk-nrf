.. _ble_llpm:

Bluetooth: LLPM
###############

.. contents::
   :local:
   :depth: 2

The Bluetooth® Low Latency Packet Mode (LLPM) sample uses the :ref:`latency_readme` and the :ref:`latency_client_readme` to showcase the LLPM proprietary Bluetooth extension from Nordic Semiconductor.
You can use it to determine the transmission latency of LLPM-enabled connections, or to compare with different connection parameters and check their influence on the results.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also supports other development kits running SoftDevice Controller variants that support LLPM (see :ref:`SoftDevice Controller proprietary feature support <sdc_proprietary_feature_support>`).
You can use any two of the development kits mentioned above and mix different development kits.

Additionally, the sample requires a connection to a computer with a serial terminal for each of the development kits.

Overview
********

The LLPM is designed for applications in which the interface response time is critical for the user.
For example, for virtual reality headsets or gaming mouse and keyboard.

The key LLPM elements are:

LLPM connection interval (1 ms)
   The connection interval defines how often the devices must listen on the radio.
   The LLPM introduces the possibility to reduce the connection interval below what is supported in Bluetooth LE.
   The lowest supported connection interval is 1 ms for one link.

Physical layer (PHY)
   Starting with Bluetooth® 5, the over-the-air data rate in Bluetooth Low Energy supports 2 Ms/s (mega symbol per second), which allows for faster transmission.
   The LLPM connection interval is only supported on *LE 2M PHY*.
   Otherwise, the SoftDevice Controller will deny the request command.

QoS connection event reports
   When reports are enabled, one report will be generated on every connection event.
   The report gives information about the quality of service of the connection event.
   The values in the report are used to describe the quality of links.
   For parameter descriptions, see :c:type:`sdc_hci_vs_subevent_qos_conn_event_report_t` (in :file:`sdc_hci_vs.h`).

Transmission latency
   The definition of the latency used in this example counts the time interval from the sender's application to the GATT service of the receiver.
   It demonstrates the performance of an LLPM-enabled connection that the receiver will receive the data approximately every 1 ms.

GATT Latency Service
   To measure the transmission latency from application layers, a GATT Latency service `BT_UUID_LATENCY` is included to compute the time spent.
   When the sender writes its timestamp to the `BT_UUID_LATENC_CHAR` characteristic of the receiver, the Latency service of the receiver will automatically reply back.
   Whenever the sender receives a response, it will use its current time and the corresponding timestamp written before to estimate the round-trip time (RTT) of a writing characteristic procedure (see `Bluetooth Core Specification`_: Vol 3, Part F, 3.4.5 Writing attributes).

   .. list-table:: GATT Attributes
      :header-rows: 1

      * - Type
        - UUID
        - Property
      * - Primary service
        - BT_UUID_LATENCY
        - Read only
      * - Characteristic
        - BT_UUID_LATENCY_CHAR
        - BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE

This sample transmits data between two development kits to measure the transmission latency in between.
One of the devices is connected as a *central* and another is connected as a *peripheral*.
The performance is evaluated with the transmission latency dividing the estimated round-trip time in half (RTT / 2).

By default, the following values are used to demonstrate the interaction of the connection parameters:

.. list-table:: Default parameter values
   :header-rows: 1

   * - Parameter
     - Value
   * - Connection interval
     - 80 units (100 ms)
   * - LLPM connection interval
     - Lowest interval (1 ms)
   * - Physical layer (PHY)
     - LE 2M PHY

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/llpm`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

After programming the sample to both development kits, test it by performing the following steps:

1. Connect to both kits with a terminal emulator (for example, the `Serial Terminal app`_).
   See :ref:`test_and_optimize` for the required settings and steps.
#. Reset both kits.
#. In one of the terminal emulators, type "c" to start the application on the connected board in the central (tester) role.
#. In the other terminal emulator, type "p" to start the application in the peripheral (peer) role.
#. Observe that the kits establish a connection.

   - The central outputs the following information::

       Press any key to start measuring transmission latency

   - The peripheral outputs the following information::

       Press any key to start measuring transmission latency

#. Press a key in the terminal that is connected to the peripheral.
#. Observe the terminal connected to the peripheral.
   The latency measurements are printed in the terminal.
   The latency is expected to be around 1 ms.

       Transmission Latency: 1083 (us), CRC mismatches: 0

#. Press a key in the terminal that is connected to the central.
#. Observe the terminal connected to the peripheral.
   The measured latency on the peripheral becomes approximate 1 ms::

       Transmission Latency: 1098 (us), CRC mismatches: 0

#. Observe the central switches to standard 7.5 ms interval right after performing latency measurements.

#. Observe the terminal connected to the central.
   The latency is higher now.

   Connection interval updated: 7.5 ms
   Transmission Latency: 4592 (us), CRC mismatches: 0

.. msc::
   hscale = "1.3";
   Central,Peripheral;
   Central<<=>>Peripheral      [label="Connected"];
   Central<<=>>Peripheral      [label="Discovered GATT Latency Service"];
   Peripheral note Peripheral  [label="Press any key to start measuring transmission latency"];
   Peripheral note Peripheral  [label="Read current timestamp: s1"];
   Peripheral=>Central         [label="Write Request (timestamp: s1)"];
   Central>>Peripheral         [label="Write Response"];
   Peripheral note Peripheral  [label="Read current timestamp: s2"];
   Peripheral note Peripheral  [label="Latency = (s2 - s1) / 2"];
   Central note Central        [label="Press any key to set LLPM short connection connection interval (1 ms)"];
   Central<<=>>Peripheral      [label="Switched to LLPM connection interval"];
   Central note Central        [label="Press any key to start measuring transmission latency"];
   Central note Central        [label="Read current timestamp: m1"];
   Central=>Peripheral         [label="Write Request (timestamp: m1)"];
   Peripheral>>Central         [label="Write Response"];
   Central note Central        [label="Read current timestamp: m2"];
   Central note Central        [label="Latency = (m2 - m1) / 2"];


Sample output
=============

The result should look similar to the following output.

- For the central::

   *** Booting Zephyr OS build v3.3.99-ncs1-2858-gc9d01d05ce83 ***
   Starting Bluetooth LLPM sample
   I: SoftDevice Controller build revision:
   I: 01 2d a8 d4 0d e5 25 cf |.-....%.
   I: a3 48 8d 2f 56 e0 59 c8 |.H./V.Y.
   I: 24 df 3d 58             |$.=X
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 1.43053 Build 635768276
   I: Identity: FF:FA:56:08:E1:DD (random)
   I: HCI: version 5.4 (0x0d) revision 0x10b7, manufacturer 0x0059
   I: LMP: version 5.4 (0x0d) subver 0x10b7
   Bluetooth initialized
   LLPM mode enabled
   Choose device role - type c (central) or p (peripheral):
   Central. Starting scanning
   Scanning successfully started
   Filter does not match. Address: 1F:25:BE:AC:D4:47 (random) connectable: 0
   Filter does not match. Address: 5E:29:3A:8C:5A:8D (random) connectable: 0
   Filter does not match. Address: 2C:E8:67:EA:C9:27 (random) connectable: 1
   Filters matched. Address: C3:06:3D:25:7F:73 (random) connectable: 1
   Filter does not match. Address: EA:0E:68:82:9D:0C (random) connectable: 1
   Filter does not match. Address: 1B:CC:D4:7C:9E:31 (random) connectable: 0
   Connected as central
   Conn. interval is 1 ms
   Security changed: level 2, err: 0
   Service discovery completed
   Press any key to start measuring transmission latency
   Transmission Latency: 1037 (us), CRC mismatches: 0
   Transmission Latency: 1296 (us), CRC mismatches: 0
   Transmission Latency: 1342 (us), CRC mismatches: 0
   Transmission Latency: 1419 (us), CRC mismatches: 0
   Transmission Latency: 961 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 1296 (us), CRC mismatches: 0
   Transmission Latency: 1342 (us), CRC mismatches: 0
   Transmission Latency: 1419 (us), CRC mismatches: 0
   Transmission Latency: 961 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 1296 (us), CRC mismatches: 0
   Transmission Latency: 1342 (us), CRC mismatches: 0
   Transmission Latency: 1419 (us), CRC mismatches: 0
   Transmission Latency: 961 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 1296 (us), CRC mismatches: 0
   Transmission Latency: 1342 (us), CRC mismatches: 0
   Transmission Latency: 1419 (us), CRC mismatches: 0
   Transmission Latency: 961 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 1296 (us), CRC mismatches: 0
   Transmission Latency: 1342 (us), CRC mismatches: 0
   Transmission Latency: 1419 (us), CRC mismatches: 0
   Transmission Latency: 961 (us), CRC mismatches: 0
   Connection interval updated: 7.5 ms
   Transmission Latency: 1922 (us), CRC mismatches: 0
   Transmission Latency: 7064 (us), CRC mismatches: 0
   Transmission Latency: 13244 (us), CRC mismatches: 0
   Transmission Latency: 4592 (us), CRC mismatches: 0
   Transmission Latency: 7019 (us), CRC mismatches: 0
   Transmission Latency: 5691 (us), CRC mismatches: 0
   Transmission Latency: 4592 (us), CRC mismatches: 0
   Transmission Latency: 7019 (us), CRC mismatches: 0
   Transmission Latency: 5706 (us), CRC mismatches: 0
   Transmission Latency: 4577 (us), CRC mismatches: 0
   Transmission Latency: 7019 (us), CRC mismatches: 0
   Transmission Latency: 5706 (us), CRC mismatches: 0
   Transmission Latency: 4592 (us), CRC mismatches: 0
   Transmission Latency: 7019 (us), CRC mismatches: 0
   Transmission Latency: 5691 (us), CRC mismatches: 0
   Transmission Latency: 4592 (us), CRC mismatches: 0
   Transmission Latency: 7019 (us), CRC mismatches: 0
   Transmission Latency: 5706 (us), CRC mismatches: 0

- For the peripheral::

   *** Booting Zephyr OS build v3.3.99-ncs1-2858-gc9d01d05ce83 ***
   Starting Bluetooth LLPM sample
   I: SoftDevice Controller build revision:
   I: 01 2d a8 d4 0d e5 25 cf |.-....%.
   I: a3 48 8d 2f 56 e0 59 c8 |.H./V.Y.
   I: 24 df 3d 58             |$.=X
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 1.43053 Build 635768276
   I: Identity: C3:06:3D:25:7F:73 (random)
   I: HCI: version 5.4 (0x0d) revision 0x10b7, manufacturer 0x0059
   I: LMP: version 5.4 (0x0d) subver 0x10b7
   Bluetooth initialized
   LLPM mode enabled
   Choose device role - type c (central) or p (peripheral):
   Peripheral. Starting advertising
   Advertising successfully started
   Connection event reports enabled
   W: opcode 0x200a status 0x0d
   Connected as peripheral
   Conn. interval is 1 ms
   W: opcode 0x2032 status 0x3a
   E: Failed LE Set PHY (-5)
   Security changed: level 2, err: 0
   Service discovery completed
   Press any key to start measuring transmission latency
   Transmission Latency: 1083 (us), CRC mismatches: 0
   Transmission Latency: 1312 (us), CRC mismatches: 0
   Transmission Latency: 900 (us), CRC mismatches: 0
   Transmission Latency: 1007 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 1083 (us), CRC mismatches: 0
   Transmission Latency: 1358 (us), CRC mismatches: 0
   Transmission Latency: 915 (us), CRC mismatches: 0
   Transmission Latency: 1007 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 1098 (us), CRC mismatches: 0
   Transmission Latency: 1296 (us), CRC mismatches: 0
   Transmission Latency: 900 (us), CRC mismatches: 0
   Transmission Latency: 1007 (us), CRC mismatches: 0
   Connection interval updated: 7.5 ms
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 7247 (us), CRC mismatches: 0
   Transmission Latency: 5950 (us), CRC mismatches: 0
   Transmission Latency: 4867 (us), CRC mismatches: 0
   Transmission Latency: 7293 (us), CRC mismatches: 0
   Transmission Latency: 5996 (us), CRC mismatches: 0
   Transmission Latency: 4913 (us), CRC mismatches: 0
   Transmission Latency: 7385 (us), CRC mismatches: 0
   Transmission Latency: 6088 (us), CRC mismatches: 0
   Transmission Latency: 5004 (us), CRC mismatches: 0


Dependencies
*************

This sample uses the following |NCS| libraries:

* :ref:`latency_readme`
* :ref:`latency_client_readme`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:softdevice_controller`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :file:`include/sys/printk.h`
* :file:`include/zephyr/types.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/scan.h`
  * :file:`include/bluetooth/gatt_dm.h`


References
***********

For more information about the connection parameters that are used in this sample, see the following chapters of the `Bluetooth Core Specification`_:

* Vol 3, Part F, 3.4.5 Writing attributes
* Vol 3, Part G, 4.9.3 Write Characteristic Value
* Vol 6, Part B, 5.1.1 Connection Update Procedure
* Vol 6, Part B, 5.1.10 PHY Update Procedure
