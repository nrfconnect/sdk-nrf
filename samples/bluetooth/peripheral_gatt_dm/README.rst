.. _peripheral_gatt_dm:

.. ncs-sample::
   :title: Bluetooth: Peripheral GATT Discovery Manager

   The Peripheral GATT Discovery Manager sample demonstrates how to use the :ref:`gatt_dm_readme`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a device to connect to the peripheral, for example, a phone or a tablet with `nRF Connect for Mobile`_ or `nRF Toolbox`_.

Overview
********

When connected to another device, the sample discovers the services of the connected device and outputs the service information.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_gatt_dm`

.. include:: /includes/build_and_run_ns.txt

.. _peripheral_gatt_dm_testing:

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps.
This testing procedure assumes that you are using `nRF Connect for Mobile`_.

1. |connect_kit|
#. |connect_terminal|
#. Connect to the device from nRF Connect (the device is advertising as "Nordic Discovery Sample").
   When connected, the sample starts discovering the services of the connected device.
#. Observe that the services of the connected device are printed in the terminal emulator.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gatt_dm_readme`

In addition, it uses the following Zephyr libraries:

* :ncs-file:`zephyr:/include/zephyr/types.h`
* :ncs-file:`zephyr:/lib/libc/minimal/include/errno.h`
* :ncs-file:`zephyr:/include/zephyr/sys/printk.h`
* :ncs-file:`zephyr:/include/zephyr/sys/byteorder.h`
* :ref:`zephyr:bluetooth_api`:

  * :ncs-file:`zephyr:/include/zephyr/bluetooth/bluetooth.h`
  * :ncs-file:`zephyr:/include/zephyr/bluetooth/hci.h`
  * :ncs-file:`zephyr:/include/zephyr/bluetooth/conn.h`
  * :ncs-file:`zephyr:/include/zephyr/bluetooth/uuid.h`
  * :ncs-file:`zephyr:/include/zephyr/bluetooth/gatt.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
