.. _peripheral_gatt_dm:

Bluetooth: Peripheral GATT Discovery Manager
############################################

.. contents::
   :local:
   :depth: 2

The Peripheral GATT Discovery Manager sample demonstrates how to use the :ref:`gatt_dm_readme`.

Overview
********

When connected to another device, the sample discovers the services of the connected device and outputs the service information.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52840dk_nrf52811, nrf52833dk_nrf52833, nrf52833dk_nrf52820, nrf52dk_nrf52810

The sample also requires a device to connect to the peripheral, for example, a phone or a tablet with `nRF Connect for Mobile`_ or `nRF Toolbox`_.

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_gatt_dm`

.. include:: /includes/build_and_run.txt

.. _peripheral_gatt_dm_testing:

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps.
This testing procedure assumes that you are using `nRF Connect for Mobile`_.

1. Connect the kit to the computer using a USB cable.
   The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. |connect_terminal|
#. Connect to the device from nRF Connect (the device is advertising as "Nordic Discovery Sample").
   When connected, the sample starts discovering the services of the connected device.
#. Observe that the services of the connected device are printed in the terminal emulator.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`gatt_dm_readme`

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
