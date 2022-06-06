.. _peripheral_Proximity:

Bluetooth: Peripheral Proximity
#########################

.. contents::
   :local:
   :depth: 2


Overview
********

The Proximity Application is an example that implements the Proximity Profile,The Proximity Profile alerts the user when connected devices are too far apart.
The application includes the following services:

For the Proximity Profile:
Link Loss Service
Immediate Alert Service
TX Power Service

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840

The sample also requires a smartphone or tablet running a compatible application.
The `Testing`_ instructions refer to `nRF Connect for Mobile`_, but you can also use other similar applications (for example, `nRF Blinky`_ or `nRF Toolbox`_).


Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_lbs`

.. include:: /includes/build_and_run.txt

Minimal build
=============

You can build the sample with a minimum configuration as a demonstration of how to reduce code size and RAM usage, using the ``-DCONF_FILE='prj_minimal.conf'`` flag in your build.

See :ref:`cmake_options` for instructions on how to add this option to your build.
For example, when building on the command line, you can do so as follows:

.. code-block:: console

   west build samples/bluetooth/peripheral_Proximity -- -DCONF_FILE='prj_minimal.conf'

.. _peripheral_lbs_testing:

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps:

1. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
#. Power on the development kit.
#. Connect to the device from the nRF Connect application,LED 2 is on after connected.
   The device is advertising as ``Nordic_Proximity``.
   The services of the connected device are shown.
#. In :guilabel:` Immediate Alert Service`, Set the AlertLevel characteristic in Immediate Alert Service to 1 (enter '01' in the text box, and click the 'write' button), and observe the Mild alert(LED 1   is blinking (period 1200 msec, duty cycle: 50%)) .
#. Change the AlertLevel value to 2 and observe the High alert(LED 1 is on).
#. Change the AlertLevel value to 0 and observe LED 1 is off.
#. Set the AlertLevel characteristic in Link Loss Service to 1.
#. Simulate a link loss situation, for example by shielding the board, and observe that LED 2 is  off (disconnected) and LED 1 is is blinking (period 1200 msec, duty cycle: 50%).
#. Reconnect and observe that the LED 2 is on.
#. Set the AlertLevel characteristic in Link Loss Service to 2.
#. Simulate a link loss situation, for example by shielding the board, and observe that LED 2 is  off (disconnected) and LED 1 is is on.
#. Reconnect and observe that the LED 2 is on.
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
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
