.. _peripheral_hr_coded:

Bluetooth: Peripheral Heart Rate Monitor with Coded PHY
#######################################################

.. contents::
   :local:
   :depth: 2

The Peripheral Heart Rate Monitor with Coded PHY offers similar functionality to the :ref:`zephyr:peripheral_hr` sample from Zephyr.
However, this sample supports LE Coded PHY.

Overview
********

The sample demonstrates a basic Bluetooth LE Peripheral role functionality that exposes the Heart Rate GATT Service with LE Coded PHY support.
Once it connects to a Central device, it generates dummy heart rate values.
It can be used together with the :ref:`bluetooth_central_hr_coded` sample.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dongle_nrf52840, nrf52840dk_nrf52840

.. include:: /includes/hci_rpmsg_overlay.txt

The sample also requires:

* A device running a Heart Rate Server with LE Coded PHY support to connect to.
  For example, another development kit running the :ref:`bluetooth_central_hr_coded` sample.

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_hr_coded`

.. include:: /includes/build_and_run.txt


Testing
=======

After programming the sample to your development kit, you can test it by connecting to another development kit that runs the :ref:`bluetooth_central_hr_coded`.

1. |connect_terminal_specific|
#. Reset the kit.
#. Program the other kit with the :ref:`bluetooth_central_hr_coded` sample.
#. Wait until the Coded advertiser is detected by the Central.
   In the terminal window, check for information similar to the following::

      Connected: xx.xx.xx.xx.xx.xx (random), tx_phy 4, rx_phy 4

#. In the terminal window, observe that notifications are enabled::

      <inf> hrs: HRS notifications enabled

Dependencies
************

This sample uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``include/errno.h``
* ``include/zephyr.h``
* ``include/sys/printk.h``
* ``include/sys/byteorder.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

* ``include/bluetooth/bluetooth.h``
* ``include/bluetooth/conn.h``
* ``include/bluetooth/uuid.h``
* ``include/bluetooth/gatt.h``
* ``include/bluetooth/services/bas.h``
* ``include/bluetooth/services/hrs.h``
