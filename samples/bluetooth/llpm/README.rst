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

Testing
=======

After programming the sample to both development kits, test it by performing the following steps:

1. Connect to both kits with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Reset both kits.
#. In one of the terminal emulators, type "c" to start the application on the connected board in the central (tester) role.
#. In the other terminal emulator, type "p" to start the application in the peripheral (peer) role.
#. Observe that the kits establish a connection.

   - The central outputs the following information::

       Press any key to set LLPM short connection connection interval (1 ms)

   - The peripheral outputs the following information::

       Press any key to start measuring transmission latency

#. Press a key in the terminal that is connected to the peripheral.
#. Observe the terminal connected to the peripheral. The latency measurements are printed in the terminal.
   The latency is expected to be shorter than the default connection interval::

       Transmission Latency: 80917 (us), CRC mismatches: 0

#. Press a key in the terminal that is connected to the central.
#. Observe the connection gets updated to LLPM connection interval (1 ms) on both sides::

       Connection interval updated: LLPM (1 ms)

#. Observe the terminal connected to the peripheral.
   The measured latency on the peripheral becomes approximate 1 ms::

       Transmission Latency: 1098 (us), CRC mismatches: 0

#. Press a key in the terminal that is connected to the central.
#. Observe the terminal connected to the central.
   The measured latency on the central remains approximate 1 ms::

       Transmission Latency: 1235 (us), CRC mismatches: 0

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

   ***** Booting Zephyr OS build v1.14.99-ncs3-snapshot2-2647-gd6e67554cfeb *****
   Bluetooth initialized
   LLPM mode enabled
   Choose device role - type c (central) or p (peripheral): c
   Central. Starting scanning
   Scanning successfully started
   Connection event reports enabled
   Filter not match. Address: 08:c6:a4:e0:72:e9 (random) connectable: 0
   Filter not match. Address: 13:04:eb:f1:0b:46 (random) connectable: 0
   Filter not match. Address: 2b:74:72:c3:8f:a8 (random) connectable: 0
   Filter not match. Address: 02:ec:f9:bb:ec:27 (random) connectable: 0
   Filter not match. Address: 01:54:cf:d4:31:cd (random) connectable: 0
   Filter not match. Address: 3e:21:91:91:52:82 (random) connectable: 0
   Filter not match. Address: 08:c6:a4:e0:72:e9 (random) connectable: 0
   Filter not match. Address: 37:63:6a:ed:38:e2 (random) connectable: 0
   Filter not match. Address: 56:c6:75:17:80:d8 (random) connectable: 1
   Filters matched. Address: f9:3c:9c:d1:f6:07 (random) connectable: 1
   Connected as central
   Conn. interval is 80 units (1.25 ms/unit)
   Service discovery completed
   Press any key to set LLPM short connection interval (1 ms)
   Press any key to start measuring transmission latency
   Connection interval updated: LLPM (1 ms)
   Transmission Latency: 1235 (us), CRC mismatches: 0
   Transmission Latency: 1007 (us), CRC mismatches: 0
   Transmission Latency: 1434 (us), CRC mismatches: 0
   Transmission Latency: 1312 (us), CRC mismatches: 0
   Transmission Latency: 1220 (us), CRC mismatches: 0
   Transmission Latency: 991 (us), CRC mismatches: 0
   Transmission Latency: 1419 (us), CRC mismatches: 0
   Transmission Latency: 1281 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 991 (us), CRC mismatches: 0
   Transmission Latency: 1403 (us), CRC mismatches: 0
   Transmission Latency: 1296 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 976 (us), CRC mismatches: 0
   Transmission Latency: 1358 (us), CRC mismatches: 0
   Transmission Latency: 1281 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 976 (us), CRC mismatches: 0
   Transmission Latency: 1358 (us), CRC mismatches: 0
   Transmission Latency: 1281 (us), CRC mismatches: 0
   Transmission Latency: 1052 (us), CRC mismatches: 0
   Transmission Latency: 976 (us), CRC mismatches: 0
   Transmission Latency: 1358 (us), CRC mismatches: 0
   Transmission Latency: 1281 (us), CRC mismatches: 0

- For the peripheral::

   ***** Booting Zephyr OS build v1.14.99-ncs3-snapshot2-2647-gd6e67554cfeb *****
   Bluetooth initialized
   LLPM mode enabled
   Choose device role - type c (central) or p (peripheral): p
   Slave role. Starting advertising
   Advertising successfully started
   Connection event reports enabled
   Filter not match. Address: 1d:18:b1:84:fd:05 (random) connectable: 0
   Filter not match. Address: 00:92:3f:a6:3f:48 (random) connectable: 0
   Filter not match. Address: 02:ec:f9:bb:ec:27 (random) connectable: 0
   Filter not match. Address: 3e:21:91:91:52:82 (random) connectable: 0
   Filter not match. Address: 08:c6:a4:e0:72:e9 (random) connectable: 0
   Filter not match. Address: 13:04:eb:f1:0b:46 (random) connectable: 0
   Filter not match. Address: cb:01:1a:2d:6e:ae (random) connectable: 1
   Filter not match. Address: 5c:f2:70:c2:3f:9f (random) connectable: 1
   Connected as peripheral
   Conn. interval is 80 units (1.25 ms/unit)
   Service discovery completed
   Press any key to start measuring transmission latency
   Transmission Latency: 80917 (us), CRC mismatches: 0
   Transmission Latency: 80841 (us), CRC mismatches: 0
   Transmission Latency: 80749 (us), CRC mismatches: 0
   Transmission Latency: 80673 (us), CRC mismatches: 0
   Transmission Latency: 80596 (us), CRC mismatches: 0
   Transmission Latency: 80505 (us), CRC mismatches: 0
   Transmission Latency: 80429 (us), CRC mismatches: 0
   Transmission Latency: 80337 (us), CRC mismatches: 0
   Transmission Latency: 80261 (us), CRC mismatches: 0
   Transmission Latency: 80184 (us), CRC mismatches: 0
   Transmission Latency: 80093 (us), CRC mismatches: 0
   Transmission Latency: 80017 (us), CRC mismatches: 0
   Transmission Latency: 79940 (us), CRC mismatches: 0
   Transmission Latency: 79849 (us), CRC mismatches: 0
   Connection interval updated: LLPM (1 ms)
   Transmission Latency: 81604 (us), CRC mismatches: 0
   Transmission Latency: 30181 (us), CRC mismatches: 0
   Transmission Latency: 1098 (us), CRC mismatches: 0
   Transmission Latency: 1129 (us), CRC mismatches: 0
   Transmission Latency: 1037 (us), CRC mismatches: 0
   Transmission Latency: 930 (us), CRC mismatches: 0
   Transmission Latency: 1312 (us), CRC mismatches: 0
   Transmission Latency: 1083 (us), CRC mismatches: 0
   Transmission Latency: 1007 (us), CRC mismatches: 0


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
