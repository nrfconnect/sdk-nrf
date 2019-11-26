.. _ble_llpm:

Bluetooth: LLPM
###############

The Bluetooth Low Latency Packet Mode (LLPM) sample uses the :ref:`latency_readme` and the :ref:`latency_c_readme` to showcase the LLPM proprietary Bluetooth extension from Nordic Semiconductor.
You can use it to determine the transmission latency of LLPM-enabled connections, or to compare with different connection parameters and check their influence on the results.


Overview
********

The LLPM is designed for applications in which the interface response time is critical for the user.
For example, for virtual reality headsets or gaming mouse and keyboard.

See the following subsections for a description of the key LLPM elements.

LLPM connection interval (1 ms)
   The connection interval defines how often the devices must listen on the radio.
   The LLPM introduces the possibility to reduce the connection interval below what is supported in BLE.
   The lowest supported connection interval is 1 ms for one link.

Physical layer (PHY)
   Starting with Bluetooth 5, the over-the-air data rate in Bluetooth Low Energy supports 2 Ms/s (mega symbol per second), which allows for faster transmission.
   The LLPM connection interval is only supported on *LE 2M PHY*.
   Otherwise, the BLE controller will deny the request command.

QoS connection event reports
   When reports are enabled, one report will be generated on every connection event.
   The report gives information about the quality of service of the connection event.
   The values in the report are used to describe the quality of links.
   For parameter descriptions, see :cpp:enum:`hci_vs_evt_qos_conn_event_report_t` (in :file:`ble_controller_hci_vs.h`).

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

This sample transmits data between two boards to measure the transmission latency in between.
One of the devices is connected as a *master* and another is connected as a *slave*.
The performance is evaluated with the transmission latency dividing the estimated round-trip time in half (RTT / 2).

By default, the following values are used to demonstrates the interaction of the connection parameters:

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


Requirements
************

* Two of the following nRF52-series development kit boards:

  * |nRF52DK|
  * |nRF52840DK|
  * Other boards running BLE Controller variants that support LLPM (see :ref:`nrfxlib:ble_controller` Proprietary feature support)

  You can mix different boards.
* Connection to a computer with a serial terminal for each of the boards.


Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/llpm`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to both boards, test it by performing the following steps:

1. Connect to both boards with a terminal emulator (for example, PuTTY).
   See :ref:`putty` for the required settings.
#. Reset both boards.
#. Observe that the boards establish a connection.
   When they are connected, one of them serves as *master* and the other one as *slave*.

   - The master outputs the following information::

       Press any key to set LLPM short connection connection interval (1 ms)

   - The slave outputs the following information::

       Press any key to start measuring transmission latency

#. Press a key in the terminal that is connected to the slave.
#. Observe the terminal connected to the slave. The latency measurements are printed in the terminal.
   The latency is expected to be shorter than the default connection interval::

       Transmission Latency: 80917 (us)

#. Press a key in the terminal that is connected to the master.
#. Observe the connection gets updated to LLPM connection interval (1 ms) on both sides::

       Connection interval updated: LLPM (1 ms)

#. Observe the terminal connected to the slave.
   The measured latency on the slave becomes approximate 1 ms::

       Transmission Latency: 1098 (us)

#. Press a key in the terminal that is connected to the master.
#. Observe the terminal connected to the master.
   The measured latency on the master remains approximate 1 ms::

       Transmission Latency: 1235 (us)

.. msc::
   hscale = "1.3";
   Master,Slave;
   Master<<=>>Slave         [label="Connected"];
   Master<<=>>Slave         [label="Discovered GATT Latency Service"];
   Slave note Slave         [label="Press any key to start measuring transmission latency"];
   Slave note Slave         [label="Read current timestamp: s1"];
   Slave=>Master            [label="Write Request (timestamp: s1)"];
   Master>>Slave            [label="Write Response"];
   Slave note Slave         [label="Read current timestamp: s2"];
   Slave note Slave         [label="Latency = (s2 - s1) / 2"];
   Master note Master       [label="Press any key to set LLPM short connection connection interval (1 ms)"];
   Master<<=>>Slave         [label="Switched to LLPM connection interval"];
   Master note Master       [label="Press any key to start measuring transmission latency"];
   Master note Master       [label="Read current timestamp: m1"];
   Master=>Slave            [label="Write Request (timestamp: m1)"];
   Slave>>Master            [label="Write Response"];
   Master note Master       [label="Read current timestamp: m2"];
   Master note Master       [label="Latency = (m2 - m1) / 2"];


Sample output
=============

The result should look similar to the following output.

- For the master::

   ***** Booting Zephyr OS build v1.14.99-ncs3-snapshot2-2647-gd6e67554cfeb *****
   Bluetooth initialized
   LLPM mode enabled
   Advertising successfully started
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
   Connected as master
   Conn. interval is 80 units (1.25 ms/unit)
   QoS conn event reports: channel index 0x1f, CRC errors 0x00
   Service discovery completed
   Press any key to set LLPM short connection interval (1 ms)
   QoS conn event reports: channel index 0x07, CRC errors 0x00
   Press any key to start measuring transmission latency
   Connection interval updated: LLPM (1 ms)
   Transmission Latency: 1235 (us)
   Transmission Latency: 1007 (us)
   QoS conn event reports: channel index 0x22, CRC errors 0x00
   Transmission Latency: 1434 (us)
   Transmission Latency: 1312 (us)
   Transmission Latency: 1220 (us)
   Transmission Latency: 991 (us)
   Transmission Latency: 1419 (us)
   QoS conn event reports: channel index 0x1a, CRC errors 0x00
   Transmission Latency: 1281 (us)
   Transmission Latency: 1052 (us)
   Transmission Latency: 991 (us)
   Transmission Latency: 1403 (us)
   Transmission Latency: 1296 (us)
   Transmission Latency: 1052 (us)
   Transmission Latency: 976 (us)
   Transmission Latency: 1358 (us)
   Transmission Latency: 1281 (us)
   Transmission Latency: 1052 (us)
   Transmission Latency: 976 (us)
   QoS conn event reports: channel index 0x1d, CRC errors 0x00
   Transmission Latency: 1358 (us)
   Transmission Latency: 1281 (us)
   Transmission Latency: 1052 (us)
   Transmission Latency: 976 (us)
   Transmission Latency: 1358 (us)
   QoS conn event reports: channel index 0x10, CRC errors 0x00
   Transmission Latency: 1281 (us)

- For the slave::

   ***** Booting Zephyr OS build v1.14.99-ncs3-snapshot2-2647-gd6e67554cfeb *****
   Bluetooth initialized
   LLPM mode enabled
   Advertising successfully started
   Scanning successfully started
   Connection event reports enabled
   Filter not match. Address: 1d:18:b1:84:fd:05 (random) connectable: 0
   Filter not match. Address: 00:92:3f:a6:3f:48 (random) connectable: 0
   Filter not match. Address: 02:ec:f9:bb:ec:27 (random) connectable: 0
   Filter not match. Address: 3e:21:91:91:52:82 (random) connectable: 0
   Filter not match. Address: 08:c6:a4:e0:72:e9 (random) connectable: 0
   Filter not match. Address: 13:04:eb:f1:0b:46 (random) connectable: 0
   Filter not match. Address: cb:01:1a:2d:6e:ae (random) connectable: 1
   Filter not match. Address: 5c:f2:70:c2:3f:9f (random) connectable: 1
   Connected as slave
   Conn. interval is 80 units (1.25 ms/unit)
   QoS conn event reports: channel index 0x1f, CRC errors 0x00
   Service discovery completed
   Press any key to start measuring transmission latency
   QoS conn event reports: channel index 0x07, CRC errors 0x00
   Transmission Latency: 80917 (us)
   Transmission Latency: 80841 (us)
   Transmission Latency: 80749 (us)
   Transmission Latency: 80673 (us)
   Transmission Latency: 80596 (us)
   Transmission Latency: 80505 (us)
   Transmission Latency: 80429 (us)
   Transmission Latency: 80337 (us)
   Transmission Latency: 80261 (us)
   Transmission Latency: 80184 (us)
   Transmission Latency: 80093 (us)
   Transmission Latency: 80017 (us)
   Transmission Latency: 79940 (us)
   Transmission Latency: 79849 (us)
   Connection interval updated: LLPM (1 ms)
   Transmission Latency: 81604 (us)
   Transmission Latency: 30181 (us)
   Transmission Latency: 1098 (us)
   QoS conn event reports: channel index 0x22, CRC errors 0x00
   Transmission Latency: 1129 (us)
   Transmission Latency: 1037 (us)
   Transmission Latency: 930 (us)
   Transmission Latency: 1312 (us)
   Transmission Latency: 1083 (us)
   Transmission Latency: 1007 (us)


Dependencies
*************

This sample uses the following |NCS| libraries:

* :ref:`latency_readme`
* :ref:`latency_c_readme`

This sample uses the following `nrfxlib`_ libraries:

* :ref:`nrfxlib:ble_controller`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel`:

  * :file:`include/kernel.h`

* :file:`include/misc/printk.h`
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
