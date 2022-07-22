.. _peripheral_Proximity:

Bluetooth: Peripheral Proximity
###############################

.. contents::
   :local:
   :depth: 2


Overview
********

The Proximity Application is an example that implements the Proximity Profile.The Proximity Profile alerts the user when connected devices are too far apart.
The application includes the following services:

For the Proximity Profile:
Link Loss Service
Immediate Alert Service
TX Power Service

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a phone or tablet running a compatible application, for example `nRF Connect for Mobile`_ or `nRF Toolbox`_.


Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_proximity`

.. include:: /includes/build_and_run.txt

.. _peripheral_proximity_testing:

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps:

1. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
#. Power on the development kit.
#. Connect to the device from the nRF Connect application.
   The device is advertising as ``Nordic_Proximity``.
   The services of the connected device are shown.
#. In :guilabel:` Immediate Alert Service`, Set the AlertLevel characteristic in Immediate Alert Service to 1 (enter '01' in the text box, and click the 'write' button), and observe the Mild alert(LED 1   is blinking (period 1200 msec, duty cycle: 50%)) .
#. Change the AlertLevel value to 2 and observe the High alert(LED 1 is on).
#. Change the AlertLevel value to 0 and observe LED 1 is off.
#. Set the AlertLevel characteristic in Link Loss Service to 1.
#. Simulate a link loss situation, for example by shielding the board, and observe that LED 2 is is blinking (period 1200 msec, duty cycle: 50%).
#. Reconnect and observe that the LED 2 is off.
#. Set the AlertLevel characteristic in Link Loss Service to 2.
#. Simulate a link loss situation, for example by shielding the board, and observe that LED 2 is on.
#. Reconnect and observe that the LED 2 is off.
#. Select the TxPowerLevel characteristic in TX Power Service, click the 'read' button, and verify that it was successfully read.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``lib/libc/minimal/include/errno.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
