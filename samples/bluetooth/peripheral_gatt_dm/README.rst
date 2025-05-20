.. _peripheral_gatt_dm:

Bluetooth: Peripheral GATT Discovery Manager
############################################

.. contents::
   :local:
   :depth: 2

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

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
