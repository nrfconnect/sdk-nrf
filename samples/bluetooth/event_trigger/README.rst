.. _ble_event_trigger:

Bluetooth: Event Trigger
########################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Event Trigger sample showcases the Event Trigger feature.
It is a proprietary extension of the Bluetooth® protocol which can be used to schedule sampling, GPIO toggling, or other SW- or HW-based user activities in synchronization with individual Bluetooth LE role instances, such as a central or peripheral link.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any two of the development kits mentioned above and mix different development kits.

Additionally, the sample requires a connection to a computer with a serial terminal for each of the development kits.

Note that the Event Trigger feature involves triggering a (D)PPI task directly from the SoftDevice Controller link layer, and therefore it is expected that the application and SoftDevice Controller are running on the same core.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/event_trigger`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to both development kits, test it by performing the following steps:

1. Connect to both kits with a terminal emulator (for example, the `Serial Terminal app`_).
   See :ref:`test_and_optimize` for the required settings and steps.
#. Reset both kits.

   You will be prompted::

      Choose device role - type c (central) or p (peripheral):

#. In one of the terminal emulators, type ``c`` to start the application on the connected board in the central (tester) role.
#. In the other terminal emulator, type ``p`` to start the application in the peripheral (peer) role.
#. Observe that the kits establish a connection.

   Each kit outputs the following information::

       Press any key to switch to a 10ms connection interval and set up event trigger

#. Press a key in either terminal.
#. Observe that the link switches from a 100 ms connection interval to a 10 ms connection interval.
#. The event trigger is configured to generate a software interrupt at each connection event for the link.

   During these software interrupts, a timestamp is recorded.
#. Observe that these timestamps are printed in the terminal.

   The delta between triggers generally matches the connection interval.
   However, if the controller is not able to schedule a radio event for a given point in time, the event trigger will also be dropped.
#. Observe that the central switches back to a 100 ms connection interval.


Sample output
=============

The sample displays the data in the following format::

   *** Booting nRF Connect SDK v3.4.99-ncs1-4802-g41e34920abf4 ***
   Starting Event Trigger Sample.
   I: SoftDevice Controller build revision:
   I: ba cb 53 9c e2 c9 de b6 |..S.....
   I: 82 1d 9c b3 eb de c5 1f |........
   I: 22 d8 45 99             |".E.
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 186.21451 Build 3737772700
   I: Identity: DA:1A:7B:0E:CC:5E (random)
   I: HCI: version 5.4 (0x0d) revision 0x1174, manufacturer 0x0059
   I: LMP: version 5.4 (0x0d) subver 0x1174
   Choose device role - type c (central) or p (peripheral):
   Peripheral. Starting advertising
   Advertising successfully started
   Connected: FB:8F:24:2D:84:79 (random)
   Connection established.
   Press any key to switch to a 10ms connection interval and set up event trigger:
   Successfully configured event trigger
   Connection parameters updated. New interval: 10 ms
   Successfully configured event trigger
   Printing event trigger log.
   +-------------+----------------+----------------------------------+
   | Trigger no. | Timestamp (us) | Time since previous trigger (us) |
   |           1 |   199381378 us |                          9979 us |
   |           2 |   199391388 us |                         10010 us |
   |           3 |   199401398 us |                         10010 us |
   |           4 |   199411377 us |                          9979 us |
   |           5 |   199421387 us |                         10010 us |
   |           6 |   199431396 us |                         10009 us |
   |           7 |   199441376 us |                          9980 us |
   |           8 |   199451385 us |                         10009 us |
   |           9 |   199461395 us |                         10010 us |
   |          10 |   199471375 us |                          9980 us |
   |          11 |   199481384 us |                         10009 us |
   |          12 |   199491394 us |                         10010 us |
   |          13 |   199501373 us |                          9979 us |
   |          14 |   199511383 us |                         10010 us |
   |          15 |   199521393 us |                         10010 us |
   |          16 |   199531372 us |                          9979 us |
   |          17 |   199541382 us |                         10010 us |
   |          18 |   199551392 us |                         10010 us |
   |          19 |   199561371 us |                          9979 us |
   |          20 |   199571381 us |                         10010 us |
   |          21 |   199581390 us |                         10009 us |
   |          22 |   199591370 us |                          9980 us |
   |          23 |   199601379 us |                         10009 us |
   |          24 |   199611389 us |                         10010 us |
   |          25 |   199621399 us |                         10010 us |
   |          26 |   199631378 us |                          9979 us |
   |          27 |   199641388 us |                         10010 us |
   |          28 |   199651398 us |                         10010 us |
   |          29 |   199661377 us |                          9979 us |
   |          30 |   199671387 us |                         10010 us |
   |          31 |   199681519 us |                         10132 us |
   |          32 |   199691376 us |                          9857 us |
   |          33 |   199701385 us |                         10009 us |
   |          34 |   199711395 us |                         10010 us |
   |          35 |   199721375 us |                          9980 us |
   |          36 |   199731384 us |                         10009 us |
   |          37 |   199741394 us |                         10010 us |
   |          38 |   199751373 us |                          9979 us |
   |          39 |   199761383 us |                         10010 us |
   |          40 |   199771393 us |                         10010 us |
   |          41 |   199781372 us |                          9979 us |
   |          42 |   199791382 us |                         10010 us |
   |          43 |   199801392 us |                         10010 us |
   |          44 |   199811371 us |                          9979 us |
   |          45 |   199821381 us |                         10010 us |
   |          46 |   199831390 us |                         10009 us |
   |          47 |   199841400 us |                         10010 us |
   |          48 |   199851379 us |                          9979 us |
   |          49 |   199861389 us |                         10010 us |
   +-------------+----------------+----------------------------------+


Dependencies
*************

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
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/scan.h`
