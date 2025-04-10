.. _ble_subrating:

Bluetooth: Connection Subrating
###############################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Connection Subrating sample showcases the effect of the LE Connection Subrating feature on the duty cycle of a connection.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any combination of the development kits mentioned above in your testing setup.

Additionally, the sample requires a connection to a computer with a serial terminal for each of the development kits.

Overview
********

This sample transmits data in bursts between two development kits to measure the transmission latency using different subrating parameters.

Subrating provides a mechanism for indicating that only a specific subset of connection events will be actively used by the connected devices, with the radio not being used in other connection events.
Connection events are actively used when the peripheral listens for the event or sends data in the event.
A subrated connection can therefore have a short ACL connection interval but still exhibit a low duty cycle so that power is conserved.

When a higher bandwidth is needed, the subrate request procedure allows for connection parameter updates to be made with minimal delay.

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

By default, the following connection parameters are used:

.. list-table:: Default parameter values
   :header-rows: 1

   * - Parameter
     - Value
   * - Connection interval
     - 8 units (10 ms)
   * - Subrate factor
     - 1 or 10
   * - Continuation number
     - 0 or 1

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/subrating`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

After programming the sample to both development kits, test it by performing the following steps:

1. Connect to both kits with a terminal emulator (for example, the `Serial Terminal app`_).
   See :ref:`test_and_optimize` for the required settings and steps.
#. Reset both kits.
#. In one of the terminal emulators, type ``c`` to start the application on the connected board in the central role.
#. In the other terminal emulator, type ``p`` to start the application in the peripheral role.
#. Observe that the kits establish a connection.
#. The sample will alternate between the following configurations:

   * No subrating
   * Subrating with a factor of 10, continuation number 0
   * Subrating with a factor of 10, continuation number 1
#. Observe the difference in the duty cycle of the connection:

   * With no subrating, the link will always use a fast connection interval of 10ms.
   * With a subrate factor of 10 and no continuation events, the link will always use a slower effective connection interval of 100ms.
   * With a subrate factor of 10 and one continuation event, the link will use a fast connection interval of 10ms when data needs to be sent, and switch to a low effective connection interval of 100ms otherwise.

The sample demonstrates the ability for the peripheral to request changes to the subrating parameters.
This behavior can be changed by setting `#define SUBRATE_INITIATOR_ROLE` to `BT_CONN_ROLE_CENTRAL` in the sample.

Sample output
-------------

The result should look similar to the following output:

* For the central::

    *** Booting nRF Connect SDK v2.7.99-9539f0a1a59b ***
    *** Using Zephyr OS v3.6.99-766b306bcbe8 ***
    Starting Bluetooth Subrating sample
    I: SoftDevice Controller build revision:
    I: 0f 3f c8 4a 7e 8d b6 7a |.?.J~..z
    I: 64 7f 04 47 8b 3c 4b ae |d..G.<K.
    I: 95 16 0d 96             |....
    I: HW Platform: Nordic Semiconductor (0x0002)
    I: HW Variant: nRF52x (0x0002)
    I: Firmware: Standard Bluetooth controller (0x00) Version 15.51263 Build 3062726218
    I: Identity: EA:9C:4D:3A:2F:A8 (random)
    I: HCI: version 5.4 (0x0d) revision 0x1224, manufacturer 0x0059
    I: LMP: version 5.4 (0x0d) subver 0x1224
    Bluetooth initialized
    Choose device role - type c (central) or p (peripheral):
    Central. Starting scanning
    Scanning successfully started
    Filters matched. Address: FD:AA:B8:5A:36:E2 (random) connectable: 1
    Connected as central
    Service discovery completed
    Subrate parameters changed: Subrate Factor: 10 Continuation Number: 0
    Peripheral latency: 0 Supervision timeout: 0x01f4 (5000 ms)
    Subrate parameters changed: Subrate Factor: 10 Continuation Number: 1
    Peripheral latency: 0 Supervision timeout: 0x01f4 (5000 ms)
    Subrate parameters changed: Subrate Factor: 1 Continuation Number: 0
    Peripheral latency: 0 Supervision timeout: 0x01f4 (5000 ms)


* For the peripheral::

    *** Booting nRF Connect SDK v2.7.99-9539f0a1a59b ***
    *** Using Zephyr OS v3.6.99-766b306bcbe8 ***
    Starting Bluetooth Subrating sample
    I: SoftDevice Controller build revision:
    I: 0f 3f c8 4a 7e 8d b6 7a |.?.J~..z
    I: 64 7f 04 47 8b 3c 4b ae |d..G.<K.
    I: 95 16 0d 96             |....
    I: HW Platform: Nordic Semiconductor (0x0002)
    I: HW Variant: nRF52x (0x0002)
    I: Firmware: Standard Bluetooth controller (0x00) Version 15.51263 Build 3062726218
    I: Identity: FD:AA:B8:5A:36:E2 (random)
    I: HCI: version 5.4 (0x0d) revision 0x1224, manufacturer 0x0059
    I: LMP: version 5.4 (0x0d) subver 0x1224
    Bluetooth initialized
    Choose device role - type c (central) or p (peripheral):
    Peripheral. Starting advertising
    Advertising successfully started
    Connected as peripheral
    Service discovery completed
    Subrate parameters changed: Subrate Factor: 1 Continuation Number: 0
    Peripheral latency: 0 Supervision timeout: 0x01f4 (5000 ms)
    Simulating burst of data.
    Response round-trip time: 16174 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16876 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16876 us
    Response round-trip time: 16845 us
    Subrate parameters changed: Subrate Factor: 10 Continuation Number: 0
    Peripheral latency: 0 Supervision timeout: 0x01f4 (5000 ms)
    Simulating burst of data.
    Response round-trip time: 186218 us
    Response round-trip time: 196777 us
    Response round-trip time: 196746 us
    Response round-trip time: 196777 us
    Response round-trip time: 196746 us
    Response round-trip time: 196746 us
    Response round-trip time: 196777 us
    Response round-trip time: 196746 us
    Response round-trip time: 196777 us
    Response round-trip time: 196746 us
    Subrate parameters changed: Subrate Factor: 10 Continuation Number: 1
    Peripheral latency: 0 Supervision timeout: 0x01f4 (5000 ms)
    Simulating burst of data.
    Response round-trip time: 96069 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16845 us
    Response round-trip time: 16876 us
    Response round-trip time: 16845 us


References
***********

For more information about LE Connection Subrating, see the following chapters of the `Bluetooth Core Specification`_:

* Vol 6, Part B, 4.5.1 Connection events
* Vol 6, Part B, 5.1.19 Connection Subrate Update procedure
* Vol 6, Part B, 5.1.20 Connection Subrate Request procedure


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
