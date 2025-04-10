.. _ble_radio_notification_conn_cb:

Bluetooth: Radio Notification callback
######################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`ug_radio_notification_conn_cb` feature.
It uses the :ref:`latency_readme` and the :ref:`latency_client_readme` to showcase how you can use this feature to minimize the time between data sampling and data transmission.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any two of the development kits mentioned above and mix different development kits.

Additionally, the sample requires a connection to a computer with a serial terminal for each of the development kits.

.. note::
   The feature involves triggering a (D)PPI task directly from the SoftDevice Controller link layer.
   The application and SoftDevice Controller are supposed to be running on the same core.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/radio_notification_cb`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to both development kits, perform the following steps to test it:

1. Connect to both kits with a terminal emulator (for example, the `Serial Terminal app`_).
   See :ref:`test_and_optimize` for the required settings and steps.
#. Reset both kits.
#. In one of the terminal emulators, type ``c`` to start the application on the connected board in the central role.
#. In the other terminal emulator, type ``p`` to start the application in the peripheral role.
#. Observe that latency measurements are printed in the terminals.

Sample output
=============

The result should look similar to the following output.

For the central::

   Starting radio notification callback sample.
   I: SoftDevice Controller build revision:
   I: d6 da c7 ae 08 db 72 6f |......ro
   I: 2a a3 26 49 2a 4d a8 b3 |*.&I*M..
   I: 98 0e 07 7f             |....
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 214.51162 Build 1926957230
   I: Identity: FA:BB:79:57:D6:45 (random)
   I: HCI: version 5.4 (0x0d) revision 0x11fb, manufacturer 0x0059
   I: LMP: version 5.4 (0x0d) subver 0x11fb
   Choose device role - type c (central) or p (peripheral):
   Central. Starting scanning
   Scanning started
   Device found: CF:99:32:A5:4B:11 (random) (RSSI -43)
   Connected: CF:99:32:A5:4B:11 (random)
   Service discovery completed
   Latency: 3771 us, round trip: 53771 us
   Latency: 3741 us, round trip: 53741 us
   Latency: 3741 us, round trip: 53741 us
   Latency: 3741 us, round trip: 53741 us
   Latency: 3771 us, round trip: 53771 us
   Latency: 3771 us, round trip: 53771 us
   Latency: 3741 us, round trip: 53741 us
   Latency: 3741 us, round trip: 53741 us
   Latency: 3741 us, round trip: 53741 us
   Latency: 3771 us, round trip: 53771 us
   Latency: 3771 us, round trip: 53771 us
   Latency: 3741 us, round trip: 53741 us
   Latency: 3741 us, round trip: 53741 us

For the peripheral::

   Starting radio notification callback sample.
   I: SoftDevice Controller build revision:
   I: d6 da c7 ae 08 db 72 6f |......ro
   I: 2a a3 26 49 2a 4d a8 b3 |*.&I*M..
   I: 98 0e 07 7f             |....
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF52x (0x0002)
   I: Firmware: Standard Bluetooth controller (0x00) Version 214.51162 Build 1926957230
   I: Identity: CF:99:32:A5:4B:11 (random)
   I: HCI: version 5.4 (0x0d) revision 0x11fb, manufacturer 0x0059
   I: LMP: version 5.4 (0x0d) subver 0x11fb
   Choose device role - type c (central) or p (peripheral):
   Peripheral. Starting advertising
   Advertising started
   Connected: FA:BB:79:57:D6:45 (random)
   Service discovery completed
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3619 us, round trip: 53619 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3619 us, round trip: 53619 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us
   Latency: 3649 us, round trip: 53649 us

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
  * :file:`include/bluetooth/scan.h`
  * :file:`include/bluetooth/gatt_dm.h`
  * :file:`include/bluetooth/evt_cb.h`
