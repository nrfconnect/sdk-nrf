.. _ble_shorter_conn_intervals:

Bluetooth: Shorter Connection Intervals
#######################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the Bluetooth® Shorter Connection Intervals (SCI) feature to achieve the shortest connection intervals supported by the controller.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any combination of the development kits in your testing setup.

Additionally, the sample requires a connection to a computer with a serial terminal for each of the development kits.

Overview
********

Bluetooth SCI introduces a range of connection intervals below 7.5 ms, providing faster device responsiveness for high-performance HID devices, real-time HMI systems, and sensors.
SCI extends the connection interval range to span from 375 μs to 4.0 s, and defines the resolution to be a multiple of 125 μs.
This range is called the Extended ConnInterval Values (ECV).
The minimum supported connection interval depends on the current device and controller capabilities and can be obtained using the :c:func:`bt_conn_le_read_min_conn_interval` API.

The SCI procedure allows updating of the following connection parameters:

* Connection interval, in 125 μs units
* Subrate factor
* Peripheral latency
* Supervision timeout

This sample uses the Frame Space Update (FSU) feature to negotiate the lowest supported frame space, which is a SoftDevice Controller requirement for supporting the shortest connection intervals.
The SCI and FSU features provide a flexible and interoperable alternative to the :ref:`ble_llpm` sample, supporting lower intervals, multiple PHYs, and configurable frame space.

To measure the transmission latency from application layers, a GATT Latency service ``BT_UUID_LATENCY`` is included to compute the time spent.
When the sender writes its timestamp to the ``BT_UUID_LATENCY_CHAR`` characteristic of the receiver, the Latency service of the receiver will automatically reply.
Whenever the sender receives a response, it uses its current time and the corresponding timestamp written before to estimate the round-trip time (RTT) of a writing characteristic procedure (see `Bluetooth Core Specification`_: Vol 3, Part F, 3.4.5 Writing attributes).

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

An example GATT service ``BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE`` is included to exchange the minimum supported connection intervals of each device.
This is used by the central to obtain the peripheral controller's minimum connection interval, to ensure the interval used in the Connection Rate Update procedure is within the peripheral's capabilities.
The peripheral does not need to perform this exchange when initiating the Connection Rate Request procedure, as it provides a mechanism for the central to select an interval in the given range within its capabilities.

.. list-table:: GATT Attributes
   :header-rows: 1

   * - Type
     - UUID
     - Property
   * - Primary service
     - BT_UUID_VS_SCI_MIN_INTERVAL_SERVICE
     - Read only
   * - Characteristic
     - BT_UUID_VS_SCI_MIN_INTERVAL_CHAR
     - BT_GATT_CHRC_READ, BT_GATT_PERM_READ

The initiating device cycles through the following connection intervals:

* Minimum supported connection interval by both devices
* 1 ms
* 1.25 ms
* 2 ms
* 4 ms

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/shorter_conn_intervals`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

After programming the sample to both development kits, perform the following steps to test it:

1. Connect to both kits with a terminal emulator (for example, the `Serial Terminal app`_).
   See :ref:`test_and_optimize` for the required settings and steps.
#. Reset both kits.
#. Observe the output showing the local minimum supported connection interval.
#. In one terminal, type "y" or "n" to indicate whether this device should initiate connection interval updates.
   Both the central and peripheral device can request Shorter Connection Interval updates.
   Do the same in the other terminal, ensuring one device is set to initiate and the other is not.
#. In one of the terminal emulators, type "c" to start the application on the connected board in the central role.
#. In the other terminal emulator, type "p" to start the application in the peripheral role.
#. Observe that the kits establish a connection.
#. The devices encrypt the connection, perform service discovery, and exchange minimum supported intervals.
#. The device set will initiate the following:

   * Update to the LE 2M PHY.
   * Update the frame space for the 2M PHY to the minimum supported.
   * Cycle through different connection intervals every three seconds.
   * Print latency measurements periodically.

#. Observe the latency measurements in the terminal of the initiating device.
   The latency should decrease with shorter connection intervals.


Sample output
=============

The result should look similar to the following output:

* Initial output::

   I: Starting Bluetooth Shorter Connection Intervals sample
   I: SoftDevice Controller build revision:
   I: 4d 66 8b 3a ac a9 a6 49 |Mf.:...I
   I: 36 06 5d b6 fb ae 69 9e |6.]...i.
   I: 5f 9e 3e 73             |_.>s
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF54Lx (0x0005)
   I: Firmware: Standard Bluetooth controller (0x00) Version 77.35686 Build 2796137530
   I: HCI transport: SDC
   I: Identity: CE:CF:FB:C5:F0:47 (random)
   I: HCI: version 6.2 (0x10) revision 0x3012, manufacturer 0x0059
   I: LMP: version 6.2 (0x10) subver 0x3012
   I: Bluetooth initialized
   I: Local minimum connection interval: 750 us
   I: SCI default connection rate parameters set (min=750 us, max=10000 us)
   I: Should this device initiate connection interval updates?
   Type y (yes, this device will initiate) or n (no, peer will initiate):


* For the device initiating connection interval updates::

   I: This device will initiate connection interval updates
   I: Choose device role - type c (central) or p (peripheral):
   I: Central. Starting scanning
   I: Scanning successfully started
   I: Connected as central
   I: Conn. interval is 10000 us
   I: Security changed: level 2, err: 0
   I: Latency service discovery completed
   I: SCI service discovery completed
   I: Found SCI min interval characteristic, handle: 0x0013
   I: LE PHY updated: TX PHY LE 2M, RX PHY LE 2M
   I: Frame space updated: 63 us, PHYs: 0x02, spacing types: 0x0003, initiator: Local Host
   I: Minimum connection intervals: Local: 750 us, Peer: 750 us, Common: 750 us
   I: Transmission Latency: 9153 us
   I: Requesting new connection interval: 750 us
   I: Connection rate changed: interval 750 us, subrate factor 1, peripheral latency 0, continuation number 0, supervision timeout 4000 ms
   I: Transmission Latency: 834 us
   I: Transmission Latency: 792 us
   I: Transmission Latency: 747 us
   I: Requesting new connection interval: 1000 us
   I: Connection rate changed: interval 1000 us, subrate factor 1, peripheral latency 0, continuation number 0, supervision timeout 4000 ms
   I: Transmission Latency: 1056 us
   I: Transmission Latency: 838 us
   I: Transmission Latency: 1161 us


* For the device not initiating connection interval updates::

   I: The peer device will initiate connection interval updates
   I: Choose device role - type c (central) or p (peripheral):
   I: Peripheral. Starting advertising
   I: Advertising successfully started
   I: Connected as peripheral
   I: Conn. interval is 10000 us
   I: Security changed: level 2, err: 0
   I: Latency service discovery completed
   I: SCI service discovery completed
   I: Found SCI min interval characteristic, handle: 0x0013
   I: Minimum connection intervals: Local: 750 us, Peer: 750 us, Common: 750 us
   I: LE PHY updated: TX PHY LE 2M, RX PHY LE 2M
   I: Frame space updated: 63 us, PHYs: 0x02, spacing types: 0x0003, initiator: Peer
   I: Transmission Latency: 11551 us
   I: Connection rate changed: interval 750 us, subrate factor 1, peripheral latency 0, continuation number 0, supervision timeout 4000 ms
   I: Transmission Latency: 718 us
   I: Transmission Latency: 962 us
   I: Transmission Latency: 804 us
   I: Connection rate changed: interval 1000 us, subrate factor 1, peripheral latency 0, continuation number 0, supervision timeout 4000 ms
   I: Transmission Latency: 630 us
   I: Transmission Latency: 1056 us
   I: Transmission Latency: 838 us


References
***********

For more information about Shorter Connection Intervals and Frame Space Update, see the following chapters of the `Bluetooth Core Specification`_:

* Vol 6, Part B, 5.1.30 Frame Space Update procedure
* Vol 6, Part B, 5.1.32 Connection Rate Update procedure
* Vol 6, Part B, 5.1.33 Connection Rate Request procedure

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`latency_readme`
* :ref:`latency_client_readme`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:softdevice_controller`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:logging_api`:

  * :file:`include/logging/log.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/scan.h`
  * :file:`include/bluetooth/gatt_dm.h`
